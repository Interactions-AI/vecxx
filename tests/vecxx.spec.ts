import {
    BPEVocab,
    Counter,
    Tokens,
    TokenTransform,
    VocabMapVectorizer,
    VocabVectorizer,
    WordVocab,
    Vocab
} from '../src';
import { join } from 'path';

const testDir = join(__dirname, 'test_data');

const TEST_SENTENCE = 'My name is Dan . I am from Ann Arbor , Michigan , in Washtenaw County';
const TEST_SENTENCE_GOLD = '<GO> my name is dan . i am from ann ar@@ bor , michigan , in wash@@ ten@@ aw county <EOS>';
const TEST_IDS_GOLD = [
    1, 30, 265, 14, 2566, 5, 8, 158, 63, 10940, 525, 18637, 7, 3685, 7, 18, 14242, 1685, 2997, 4719, 2
];

const TEST_SENTENCE_GOLD_WORD_VOCAB =
    '<GO> my name is dan . i am from ann arbor , michigan , in washtenaw county <EOS>';
const TEST_IDS_GOLD_WORD_VOCAB = [1, 16, 17, 14, 10, 5, 12, 6, 11, 7, 8, 4, 15, 4, 13, 18, 9, 2];

const COUNTS: Counter = {
    washtenaw: 1,
    michigan: 1,
    dan: 1,
    '.': 1,
    my: 1,
    is: 1,
    county: 1,
    name: 1,
    from: 1,
    i: 1,
    am: 1,
    in: 1,
    ann: 1,
    ',': 2,
    arbor: 1
};
const VOCAB_LIST: Tokens = [
    'arbor',
    ',',
    'ann',
    'in',
    '.',
    'my',
    'is',
    'from',
    'dan',
    'washtenaw',
    'michigan',
    'am',
    'name',
    'county',
    'i'
];

const toLower: TokenTransform = (s: string) => s.toLowerCase();

describe('vecxx', () => {
    describe.each([
        ['bpe', TEST_SENTENCE_GOLD, TEST_IDS_GOLD, 2566, 3685],
        ['word', TEST_SENTENCE_GOLD_WORD_VOCAB, TEST_IDS_GOLD_WORD_VOCAB, 10, 15]
    ])('Vocab(%s)', (type, goldSentence, expectedIds, danIndex, michiganIndex) => {
        let vocab: Vocab;
        beforeEach(() => {
            if (type === 'bpe') {
                vocab = new BPEVocab(join(testDir, 'vocab.30k'), join(testDir, 'codes.30k'));
            } else {
                vocab = new WordVocab(COUNTS);
            }
            expect(vocab).toBeDefined();
        });

        describe('lookup', () => {
            it('lookup with transform', () => {
                const toks = goldSentence.split(/\s+/);
                const ids = toks.map((s) => vocab.lookup(s, toLower));
                expect(ids).toEqual(expectedIds);
            });

            it('lookup, with a mutating func', () => {
                const lower = vocab.lookup('Dan', toLower);
                expect(lower).toEqual(danIndex);

                const upper = vocab.lookup('Dan', () => 'michigan');
                expect(upper).toEqual(michiganIndex);
            });
        });

        describe('VocabVectorizer', () => {
            it('can create with defaults', () => {
                const vectorizer = new VocabVectorizer(vocab);
                expect(vectorizer).toBeDefined();
            });

            it('convertToPieces', () => {
                const vectorizer = new VocabVectorizer(vocab, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const sentence = vectorizer.convertToPieces(TEST_SENTENCE.split(/\s+/));
                expect(sentence).toBeDefined();
                expect(sentence.join(' ')).toEqual(goldSentence);
            });

            it('convertToIds', () => {
                const vectorizer = new VocabVectorizer(vocab, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const { ids, size } = vectorizer.convertToIds(TEST_SENTENCE.split(/\s+/));
                expect(ids).toEqual(expectedIds);
                expect(size).toEqual(expectedIds.length);
            });

            it('convertToIds, with maxLength', () => {
                const vectorizer = new VocabVectorizer(vocab, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const { ids, size } = vectorizer.convertToIds(TEST_SENTENCE.split(/\s+/), 128);
                expect(ids.slice(0, size)).toEqual(expectedIds);
                expect(ids.slice(size).reduce((sum, v) => sum + v, 0)).toEqual(0);
                expect(size).toEqual(expectedIds.length);
            });
        });

        describe('VocabMapVectorizer', () => {
            it('can create with defaults', () => {
                const vectorizer = new VocabMapVectorizer(vocab);
                expect(vectorizer).toBeDefined();
            });

            it('convertToPieces', () => {
                const vectorizer = new VocabMapVectorizer(vocab, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const tokenMaps = TEST_SENTENCE.split(/\s+/).map((text) => ({ text }));
                const sentence = vectorizer.convertToPieces(tokenMaps).join(' ');
                expect(sentence).toEqual(goldSentence);
            });

            it('convertToIds', () => {
                const vectorizer = new VocabMapVectorizer(vocab, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const tokenMaps = TEST_SENTENCE.split(/\s+/).map((text) => ({ text }));
                const { ids, size } = vectorizer.convertToIds(tokenMaps);
                expect(ids).toEqual(expectedIds);
                expect(size).toEqual(expectedIds.length);
            });

            it('convertToIds, with maxLength', () => {
                const vectorizer = new VocabMapVectorizer(vocab, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const tokenMaps = TEST_SENTENCE.split(/\s+/).map((text) => ({ text }));
                const { ids, size } = vectorizer.convertToIds(tokenMaps, 128);
                expect(ids.slice(0, size)).toEqual(expectedIds);
                expect(ids.slice(size).reduce((sum, v) => sum + v, 0)).toEqual(0);
                expect(size).toEqual(expectedIds.length);
            });
        });
    });

    describe('WordVocab w/array', () => {
        let vocab: Vocab;
        beforeEach(() => {
            vocab = new WordVocab(VOCAB_LIST);
            expect(vocab).toBeDefined();
        });

        it('test vocab list', () => {
            const vectorizer = new VocabVectorizer(vocab, {
                transform: toLower,
                emitBeginToken: ['<GO>'],
                emitEndToken: ['<EOS>']
            });
            const sentence = vectorizer.convertToPieces(TEST_SENTENCE.split(/\s+/));
            expect(sentence).toBeDefined();
            expect(sentence.join(' ')).toEqual(TEST_SENTENCE_GOLD_WORD_VOCAB);
        });
    });
});
