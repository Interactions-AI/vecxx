## vecxx

[![Python Build Status][build-status-py-img]][build-status-py-link]
[![Node.js Build Status][build-status-node-img]][build-status-node-link]

C++ implementations of vectorizers to convert strings to integers.  There are bindings available in Python and JS/TS.

This includes a straighforward C++ implementation of common approaches to vectorization to integers, required for most types of DNNs to convert sentences into tensors.  It also supports native [subword BPE](https://github.com/rsennrich/subword-nmt) based on [fastBPE](https://github.com/glample/fastBPE) with additional support for preprocessing transforms of strings during decode, either as native functors or from the bound languages.  It also supports extra (special) tokens that can be passed through.

To support fast load times, `vecxx` vocabs can be compiled to a perfect hash and persisted to disk.  When constructing the objects from the compiled directory, the loading will use memory mapping.  For a BPE vocab, a non-compiled load that takes 24 milliseconds, may take only 200 microseconds after compilation.

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

### Vocab compilation

```c++
  std::string vocab_file(argv[1]);
  std::string codes_file(argv[2]);
  auto v = new BPEVocab(vocab_file, codes_file);
  v->compile_vocab(compiled_dir);
  delete v;
  return 0;
```

```

## Python bindings

The Python bindings are written with [pybind11](https://github.com/pybind/pybind11).

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

>>> import vecxx
>>> b = vecxx.BPEVocab('/data/reddit/vocab.30k', '/data/reddit/codes.30k', extra_tokens=["[CLS]", "[MASK]"])
>>> b.compile('blah')
Traceback (most recent call last):
  File "<stdin>", line 1, in <module>
AttributeError: 'vecxx.BPEVocab' object has no attribute 'compile'
>>> b.compile_vocab('blah')
Creating blah
Creating compiled vocab 
creating blah/ph-vocab
Creating compiled codes blah/ph-codes
creating blah/ph-codes
creating blah/ph-rcodes

>>> b2 = vecxx.BPEVocab('blah', 'blah')
file blah is a directory.  Assuming mmap
Reallocating
file blah is a directory.  Assuming mmap
Reallocating
Reallocating

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

[build-status-py-img]: https://github.com/dpressel/vecxx/workflows/python/badge.svg?branch=main&event=push
[build-status-py-link]: https://github.com/dpressel/vecxx/actions?query=workflow%3A%22python%22
[build-status-node-img]: https://github.com/dpressel/vecxx/workflows/node.js/badge.svg?branch=main&event=push
[build-status-node-link]: https://github.com/dpressel/vecxx/actions?query=workflow%3A%22node.js%22

