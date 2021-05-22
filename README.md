## vecxx

C++ implementations of vectorizers to convert strings to integers.  There are bindings available in Python and JS/TS.

This includes a straighforward C++ implementation of common approaches to vectorization to integers, required for most types of DNNs to convert sentences into tensors.  It also supports native [subword BPE](https://github.com/rsennrich/subword-nmt) based on [fastBPE](https://github.com/glample/fastBPE) with additional support for preprocessing transforms of strings during decode, either as native functors or from the bound languages.  It also supports extra (special) tokens that can be passed through.

## Python bindings

The Python bindings are written with [pybind11](https://github.com/pybind/pybind11).

### Using BPE vectorizer from Python


Converting sentences to lower-case subword BPE tokens as integers from the vocabulary.
Note that a python native string transform can be used to transform each token prior to subword tokenization.
Tokens from either the BPE vocab or special tokens (like `<GO>` and `<EOS>`) can be applied to the beginning and end of the sequence.
If a second argument is provided to `convert_to_ids` this will indicate a padded length required for the tensor

```python

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

## JS/TS bindings

TODO

