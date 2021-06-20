## vecxx

[![Python Build Status][build-status-py-img]][build-status-py-link]
[![Node.js Build Status][build-status-node-img]][build-status-node-link]

Fast C++ implementations of vectorizers for machine learning models, with bindings in Python and JS/TS.

This includes a straighforward, cross-platform C++ implementation of common approaches to vectorization to integers, required for most types of DNNs to convert sentences into tensors.  It also supports native [subword BPE](https://github.com/rsennrich/subword-nmt) based on [fastBPE](https://github.com/glample/fastBPE) with additional support for preprocessing transforms of strings during decode, either as native functors or from the bound languages.  It also supports extra (special) tokens that can be passed through.

For BPE, the library supports execution of models that were trained either using subword-nmt or fastBPE (or any other system which shares the file format).

To support fast load times, `vecxx` vocabs can be compiled to a perfect hash and persisted to disk.  When constructing the objects from the compiled directory, the loading will use memory mapping.  For a BPE vocab, a non-compiled load that takes 24 milliseconds, may take only 200 microseconds after compilation.

### Handling of prefix and suffix tokens

The API supports passing in a vector of tokens for prefixing the data, and for suffixing.  These are
passed as a `[]` in Python, and `std::vector<std::string>` in C++.  Note that for prefixing, the prefix tokens will always be present, but when converting to integer ids, if the output buffer is presized and is too short to fill the tokens, the library currently will not overwrite the last values in the buffer to the end tokens.  The rationale is that, typically an end suffix will be something marking an end-of-sentence or utterance, but since the result is truncated, the end was not actually reached.  Future versions of the library may change this

### Handling of multiple input sentences

The library supports converting multiple sentences into a "stack" of tokens with a fixed output size.  This may be useful for batching or using N-best lists as input.  The results from the ID stack are contiguous 1D arrays.  They will typically be reshaped in user code:

```python
nbests = [t.split() for t in text]
ids, lengths = vec.convert_to_ids_stack(nbests, len(nbests)*args.mxlen)
ids = np.array(ids).reshape(1, len(nbests), -1)
example = {args.feature: ids, 'lengths': np.array(lengths).reshape(1, -1)}
```
## C++

```c++
#include "vecxx/vecxx.h"

std::vector<std::string> SENTENCE = {
	"My", "name", "is", "Dan", ".", "I", "am", "from", "Ann", "Arbor",
	",", "Michigan", ",", "in", "Washtenaw", "County"
};

int main(int argc, char** argv) {
    std::string vocab_file(argv[1]);
    std::string codes_file(argv[2]);
    auto v = new BPEVocab(vocab_file, codes_file);
    VocabVectorizer vec(v);
    for (auto p : vec.convert_to_pieces(SENTENCE)) {
    	std::cout << p << std::endl;
    }

    std::vector<int> ids;
    int l;
    std::tie(ids, l) = vec.convert_to_ids(SENTENCE);
    for (auto p : ids) {
    	std::cout << p << std::endl;
    }

    delete v;
    return 0;
}
```

### Vocab compilation

The initial load of the vocab is typically relatively fast, as it just reads in text files.
However, if we need much lower latency, we can optionally compile the internal data structures to memory-mapped perfect hashes.

Once compiled pass the compiled target folder in as both the `vocab_file` and `codes_file`.

```c++
  auto v = new BPEVocab(vocab_file, codes_file);
  v->compile_vocab(compiled_dir);
  delete v;
  return 0;
```

## Python bindings

The Python bindings are written with [pybind11](https://github.com/pybind/pybind11).

You can install the python bindings using `pip`:

```
pip install vecxx
```

### Using BPE vectorizer from Python


Converting sentences to lower-case subword BPE tokens as integers from the vocabulary.
Note that a python native string transform can be used to transform each token prior to subword tokenization.
Tokens from either the BPE vocab or special tokens (like `<GO>` and `<EOS>`) can be applied to the beginning and end of the sequence.
If a second argument is provided to `convert_to_ids` this will indicate a padded length required for the tensor

```python
    from vecxx import *
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    padd_vec, unpadded_len = vec.convert_to_ids("My name is Dan . I am from Ann Arbor , Michigan , in Washtenaw County".split(), 256)
```
The result of this will be:
```
[1, 30, 265, 14, 2566, 5, 8, 158, 63, 10940, 525, 18637, 7, 3685, 7, 18, 14242, 1685, 2997, 4719, 2, 0, ..., 0]
```

### Vocab compilation


As in the C++ example, we can optionally compile the internal data structures to memory-mapped perfect hashes.  Once compiled they can be read in the same way.

```python
>>> import vecxx
>>> b = vecxx.BPEVocab('/data/reddit/vocab.30k', '/data/reddit/codes.30k', extra_tokens=["[CLS]", "[MASK]"])
>>> b.compile_vocab('blah')
>>> b2 = vecxx.BPEVocab('blah', 'blah')
```

## JS/TS bindings

The Javascript bindings are provided by using the [Node-API](https://nodejs.org/api/n-api.html) API.

A thin TypeScript wrapper provides a typed API that closely matches the underlying (and Python) APIs.

### Using BPE vectorizer from TypeScript

```typescript
import { BPEVocab, VocabVectorizer } from 'vecxx';
import { join } from 'path';

const testDir = join(__dirname, 'test_data');
const bpe = new BPEVocab(join(testDir, 'vocab.30k'), join(testDir, 'codes.30k'));
const vectorizer = new VocabVectorizer(bpe, {
    transform: (s: string) => s.toLowerCase(),
    emitBeginToken: ['<GO>'],
    emitEndToken: ['<EOS>']
});
const sentence = `My name is Dan . I am from Ann Arbor , Michigan , in Washtenaw County`;
const { ids, size } = vectorizer.convertToIds(sentence.split(/\s+/), 256);
```

## Docker

Sample `Dockerfile`s are provided that can be used for sandbox development/testing.

```bash
docker build -t vecxx-python -f py.Dockerfile .
docker run -it vecxx-python
```

```bash
docker build -t vecxx-node -f node.Dockerfile .
docker run -it vecxx-node
# ...
var vecxx = require('dist/index.js')
```

[build-status-py-img]: https://github.com/dpressel/vecxx/actions/workflows/python.yml/badge.svg?branch=main&event=push
[build-status-py-link]: https://github.com/dpressel/vecxx/actions?query=workflow%3A%22python%22
[build-status-node-img]: https://github.com/dpressel/vecxx/actions/workflows/node.js.yml/badge.svg?branch=main&event=push
[build-status-node-link]: https://github.com/dpressel/vecxx/actions?query=workflow%3A%22node.js%22

