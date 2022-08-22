import os
import pytest
import numpy as np
from vecxx import *

TEST_DATA = os.path.join(os.path.realpath(os.path.dirname(__file__)), "test_data")
TEST_SENTENCE = "My name is Dan . I am from Ann Arbor , Michigan , in Washtenaw County"
TEST_SENTENCE_GOLD = "<GO> my name is dan . i am from ann ar@@ bor , michigan , in wash@@ ten@@ aw county <EOS>"
TEST_IDS_GOLD = [1, 30, 265, 14, 2566, 5, 8, 158, 63, 10940, 525, 18637, 7, 3685, 7, 18, 14242, 1685, 2997, 4719, 2]
TEST_N_SENTENCES = ["My name is Dan .", "I am from Ann Arbor , Michigan .", "in Washtenaw County"]
TEST_N_IDS_GOLD = [
    [1, 30, 265, 14, 2566, 5, 2],
    [1, 8, 158, 63, 10940, 525, 18637, 7, 3685, 5, 2],
    [1, 18, 14242, 1685, 2997, 4719, 2]
]
TEST_REVERSE_GOLD = ['', 'my', 'name', 'is', 'dan', '.', 'i', 'am', 'from', 'ann', 'ar@@', 'bor', ',', 'michigan', ',', 'in', 'wash@@', 'ten@@', 'aw', 'county', '']
def test_no_vocab():
    caught = False
    try:
        bpe = BPEVocab(
            vocab_file="i dont exist",
            codes_file=os.path.join(TEST_DATA, "codes.30k")
        )
    except Exception as e:
        caught = True
    assert caught == True

def test_no_codes():
    caught = False
    try:
        bpe = BPEVocab(
            vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
            codes_file="i dont exist"
        )
    except Exception as e:
        caught = True
    assert caught == True 

def test_pieces():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    sentence = ' '.join(vec.convert_to_pieces(TEST_SENTENCE.split()))
    assert sentence == TEST_SENTENCE_GOLD

def test_pieces_map():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    vec = VocabMapVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    map_tokens = [{"text": s} for s in TEST_SENTENCE.split()]
    sentence = ' '.join(vec.convert_to_pieces(map_tokens))
    assert sentence == TEST_SENTENCE_GOLD
    
    
def test_bpe_lookup():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    toks = TEST_SENTENCE_GOLD.split()
    ids = [bpe.lookup(s, str.lower) for s in toks]
    assert ids == TEST_IDS_GOLD

def test_compile():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    compiled_path = os.path.join(TEST_DATA, "vocab.30k.ph")
    bpe.compile_vocab(compiled_path)
    bpe = BPEVocab(
        vocab_file=compiled_path,
        codes_file=compiled_path
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    v, l = vec.convert_to_ids(TEST_SENTENCE.split())
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)

def test_ids():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    v, l = vec.convert_to_ids(TEST_SENTENCE.split())
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)

    v, l = vec.convert_to_ids(TEST_SENTENCE.split(), 128)
    assert v[:l] == TEST_IDS_GOLD
    assert np.sum(v[l+1:]) == 0
    assert l == len(TEST_IDS_GOLD)

    v, l = vec.convert_to_ids(TEST_SENTENCE.split(), 5)
    assert v == TEST_IDS_GOLD[:5]
    assert l == 5

def test_ids_reverse():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    v, l = vec.convert_to_ids(TEST_SENTENCE.split())
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)
    tok = []
    for i in v:
        tok.append(bpe.rlookup(i))
    assert tok == TEST_REVERSE_GOLD

def test_ids_reverse_ph():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    compiled_path = os.path.join(TEST_DATA, "vocab.30k.ph")
    bpe.compile_vocab(compiled_path)
    bpe = BPEVocab(
        vocab_file=compiled_path,
        codes_file=compiled_path
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    v, l = vec.convert_to_ids(TEST_SENTENCE.split())
    decoded = vec.decode(v)
    assert decoded == TEST_SENTENCE.lower()
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)

def test_ids_stack():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )
    vec = VocabVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])

    nv, nl = vec.convert_to_ids_stack([t.split() for t in TEST_N_SENTENCES], 12)
    nv = np.array(nv).reshape((len(TEST_N_SENTENCES), 12))
    for v, l, t in zip(nv, nl, TEST_N_IDS_GOLD):
        assert len(v[:l]) == len(t)
        assert all([a == b for a, b in zip(v[:l], t)])

    nv, nl = vec.convert_to_ids_stack([t.split() for t in TEST_N_SENTENCES], 5)
    nv = np.array(nv).reshape((len(TEST_N_SENTENCES), 5))
    for v, l, t in zip(nv, nl, TEST_N_IDS_GOLD):
        assert len(v[:l]) == 5
        assert all([a == b for a, b in zip(v[:l], t[:5])])

def test_ids_map():
    bpe = BPEVocab(
        vocab_file=os.path.join(TEST_DATA, "vocab.30k"),
        codes_file=os.path.join(TEST_DATA, "codes.30k")
    )

    vec = VocabMapVectorizer(bpe, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    map_tokens = [{"text": s} for s in TEST_SENTENCE.split()]

    v, l = vec.convert_to_ids(map_tokens)
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)

    v, l = vec.convert_to_ids(map_tokens, 128)
    assert v[:l] == TEST_IDS_GOLD
    assert np.sum(v[l+1:]) == 0
    assert l == len(TEST_IDS_GOLD)

