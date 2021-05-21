import os
import pytest
import numpy as np
from vecxx import *

TEST_DATA = os.path.join(os.path.realpath(os.path.dirname(__file__)), "test_data")
TEST_SENTENCE = "My name is Dan . I am from Ann Arbor , Michigan , in Washtenaw County"
TEST_SENTENCE_GOLD = "<GO> my name is dan . i am from ann arbor , michigan , in washtenaw county <EOS>"
TEST_IDS_GOLD = [1, 9, 16, 10, 12, 8, 18, 15, 11, 6, 4, 5, 14, 5, 7, 13, 17, 2]
COUNTS = {'washtenaw': 1, 'michigan': 1, 'dan': 1, '.': 1, 'my': 1, 'is': 1, 'county': 1, 'name': 1, 'from': 1, 'i': 1, 'am': 1, 'in': 1, 'ann': 1, ',': 2, 'arbor': 1}
VOCAB_LIST = ['arbor', ',', 'ann', 'in', '.', 'my', 'is', 'from', 'dan', 'washtenaw', 'michigan', 'am', 'name', 'county', 'i']

def test_vocab_list():
    words = WordVocab(
        VOCAB_LIST
    )
    vec = VocabVectorizer(words, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    sentence = ' '.join(vec.convert_to_pieces(TEST_SENTENCE.split()))
    assert sentence == TEST_SENTENCE_GOLD


def test_pieces():
    words = WordVocab(
        COUNTS
    )
    vec = VocabVectorizer(words, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    sentence = ' '.join(vec.convert_to_pieces(TEST_SENTENCE.split()))
    assert sentence == TEST_SENTENCE_GOLD

def test_pieces_map():
    words = WordVocab(
        COUNTS
    )
    vec = VocabMapVectorizer(words, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    map_tokens = [{"text": s} for s in TEST_SENTENCE.split()]
    sentence = ' '.join(vec.convert_to_pieces(map_tokens))
    assert sentence == TEST_SENTENCE_GOLD

def test_word_lookup():
    words = WordVocab(
        COUNTS
    )
    toks = TEST_SENTENCE_GOLD.split()
    ids = [words.lookup(s, str.lower) for s in toks]
    assert ids == TEST_IDS_GOLD

def test_ids():
    words = WordVocab(
        COUNTS
    )
    vec = VocabVectorizer(words, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    v, l = vec.convert_to_ids(TEST_SENTENCE.split())
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)

    v, l = vec.convert_to_ids(TEST_SENTENCE.split(), 128)
    assert v[:l] == TEST_IDS_GOLD
    assert np.sum(v[l+1:]) == 0
    assert l == len(TEST_IDS_GOLD)

def test_ids_map():
    words = WordVocab(
        COUNTS
    )

    vec = VocabMapVectorizer(words, transform=str.lower, emit_begin_tok=["<GO>"], emit_end_tok=["<EOS>"])
    map_tokens = [{"text": s} for s in TEST_SENTENCE.split()]

    v, l = vec.convert_to_ids(map_tokens)
    assert v == TEST_IDS_GOLD
    assert l == len(TEST_IDS_GOLD)

    v, l = vec.convert_to_ids(map_tokens, 128)
    assert v[:l] == TEST_IDS_GOLD
    assert np.sum(v[l+1:]) == 0
    assert l == len(TEST_IDS_GOLD)
