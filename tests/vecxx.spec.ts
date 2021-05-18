import { BPEVocab, VocabVectorizer, TokenTransform, VocabMapVectorizer } from '../src';
import { join } from 'path';

const testDir = join(__dirname, 'test_data');

const TEST_SENTENCE = 'My name is Dan . I am from Ann Arbor , Michigan , in Washtenaw County';
const TEST_SENTENCE_GOLD = '<GO> my name is dan . i am from ann ar@@ bor , michigan , in wash@@ ten@@ aw county <EOS>';
const TEST_IDS_GOLD = [
    1, 30, 265, 14, 2566, 5, 8, 158, 63, 10940, 525, 18637, 7, 3685, 7, 18, 14242, 1685, 2997, 4719, 2
];

const toLower: TokenTransform = (s: string) => s.toLowerCase();

describe('vecxx', () => {
    describe('BPEVocab', () => {
        let bpe: BPEVocab;
        beforeEach(() => {
            bpe = new BPEVocab(join(testDir, 'vocab.30k'), join(testDir, 'codes.30k'));
            expect(bpe).toBeDefined();
        });

        describe('lookup', () => {
            it('lookup with transform', () => {
                const toks = TEST_SENTENCE_GOLD.split(/\s+/);
                const ids = toks.map((s) => bpe.lookup(s, toLower));
                expect(ids).toEqual(TEST_IDS_GOLD);
            });

            it('lookup, with a mutating func', () => {
                const lower = bpe.lookup('Dan', toLower);
                expect(lower).toEqual(2566);

                const upper = bpe.lookup('Dan', () => 'michigan');
                expect(upper).toEqual(3685);
            });
        });
    });

    describe('Vectorizers', () => {
        let bpe: BPEVocab;
        beforeEach(() => {
            bpe = new BPEVocab(join(testDir, 'vocab.30k'), join(testDir, 'codes.30k'));
            expect(bpe).toBeDefined();
        });

        describe('VocabVectorizer', () => {
            it('can create with defaults', () => {
                const vectorizer = new VocabVectorizer(bpe);
                expect(vectorizer).toBeDefined();
            });

            it('convertToPieces', () => {
                const vectorizer = new VocabVectorizer(bpe, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const sentence = vectorizer.convertToPieces(TEST_SENTENCE.split(/\s+/));
                expect(sentence).toBeDefined();
                expect(sentence.join(' ')).toEqual(TEST_SENTENCE_GOLD);
            });

            it('convertToIds', () => {
                const vectorizer = new VocabVectorizer(bpe, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const { ids, size } = vectorizer.convertToIds(TEST_SENTENCE.split(/\s+/));
                expect(ids).toEqual(TEST_IDS_GOLD);
                expect(size).toEqual(TEST_IDS_GOLD.length);
            });

            it('convertToIds, with maxLength', () => {
                const vectorizer = new VocabVectorizer(bpe, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const { ids, size } = vectorizer.convertToIds(TEST_SENTENCE.split(/\s+/), 128);
                expect(ids.slice(0, size)).toEqual(TEST_IDS_GOLD);
                expect(ids.slice(size).reduce((sum, v) => sum + v, 0)).toEqual(0);
                expect(size).toEqual(TEST_IDS_GOLD.length);
            });
        });

        describe('VocabMapVectorizer', () => {
            it('can create with defaults', () => {
                const vectorizer = new VocabMapVectorizer(bpe);
                expect(vectorizer).toBeDefined();
            });

            it('convertToPieces', () => {
                const vectorizer = new VocabMapVectorizer(bpe, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const tokenMaps = TEST_SENTENCE.split(/\s+/).map((text) => ({ text }));
                const sentence = vectorizer.convertToPieces(tokenMaps).join(' ');
                expect(sentence).toEqual(TEST_SENTENCE_GOLD);
            });

            it('convertToIds', () => {
                const vectorizer = new VocabMapVectorizer(bpe, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const tokenMaps = TEST_SENTENCE.split(/\s+/).map((text) => ({ text }));
                const { ids, size } = vectorizer.convertToIds(tokenMaps);
                expect(ids).toEqual(TEST_IDS_GOLD);
                expect(size).toEqual(TEST_IDS_GOLD.length);
            });

            it('convertToIds, with maxLength', () => {
                const vectorizer = new VocabMapVectorizer(bpe, {
                    transform: toLower,
                    emitBeginToken: ['<GO>'],
                    emitEndToken: ['<EOS>']
                });
                const tokenMaps = TEST_SENTENCE.split(/\s+/).map((text) => ({ text }));
                const { ids, size } = vectorizer.convertToIds(tokenMaps, 128);
                expect(ids.slice(0, size)).toEqual(TEST_IDS_GOLD);
                expect(ids.slice(size).reduce((sum, v) => sum + v, 0)).toEqual(0);
                expect(size).toEqual(TEST_IDS_GOLD.length);
            });
        });
    });
});
