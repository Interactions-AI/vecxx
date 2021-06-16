import * as bindings from 'bindings';

const vecxx = bindings('vecxx');
const {
    Vocab: VocabBinding,
    VocabVectorizer: VocabVectorizerBinding,
    VocabMapVectorizer: VocabMapVectorizerBinding
} = vecxx;

export type Token = string;
export type Tokens = Token[];
export type TokenIds = { ids: number[]; size: number };
export type Counter = Record<string, number>;

export type TokenTransform = (s: Token) => Token;
const identityTransform: TokenTransform = (s: Token) => s;

export abstract class Vocab {
    protected constructor(public readonly binding: any) {}

    public lookup(token: string, transform?: TokenTransform): number {
        return this.binding.lookup(token, transform ?? identityTransform);
    }
}

export class BPEVocab extends Vocab {
    constructor(vocabFile: string, codesFile: string) {
        super(new VocabBinding('bpe', vocabFile, codesFile));
    }
}

export class WordVocab extends Vocab {
    /**
     * @param vocab can be a filename, an array of Tokens or a Counter record
     */
    constructor(vocab: string | Tokens | Counter) {
        super(
            new VocabBinding(
                Array.isArray(vocab) ? 'word-tokens' : typeof vocab === 'string' ? 'word-file' : 'word-counter',
                vocab
            )
        );
    }
}

export interface Vectorizer {
    convertToPieces(tokens: Tokens): Tokens;

    convertToIds(tokens: Tokens, maxLength?: number): TokenIds;
}

interface VocabVectorizerOptions {
    transform?: TokenTransform;
    emitBeginToken?: Tokens;
    emitEndToken?: Tokens;
}

export class VocabVectorizer implements Vectorizer {
    private proxy: any;

    constructor(private readonly vocab: Vocab, private readonly options?: VocabVectorizerOptions) {
        this.proxy = new VocabVectorizerBinding(
            vocab.binding,
            options?.transform ?? identityTransform,
            options?.emitBeginToken ?? [],
            options?.emitEndToken ?? []
        );
    }

    public convertToPieces(tokens: Tokens): Tokens {
        return this.proxy.convertToPieces(tokens);
    }

    public convertToIds(tokens: Tokens, maxLength = 0): TokenIds {
        return this.proxy.convertToIds(tokens, maxLength) as TokenIds;
    }
}

export type TokenMap = Record<string, Token>;

export interface MapVectorizer {
    convertToPieces(tokenMaps: TokenMap[]): Tokens;

    convertToIds(tokenMaps: TokenMap[], maxLength?: number): TokenIds;
}

interface VocabMapVectorizerOptions extends VocabVectorizerOptions {
    fields?: string[];
    delim?: string;
}

export class VocabMapVectorizer implements MapVectorizer {
    private proxy: any;

    constructor(private readonly vocab: BPEVocab, private readonly options?: VocabMapVectorizerOptions) {
        this.proxy = new VocabMapVectorizerBinding(
            vocab.binding,
            options?.transform ?? identityTransform,
            options?.emitBeginToken ?? [],
            options?.emitEndToken ?? [],
            options?.fields ?? ['text'],
            options?.delim ?? '~~'
        );
    }

    public convertToPieces(tokenMaps: TokenMap[]): Tokens {
        return this.proxy.convertToPieces(tokenMaps);
    }

    public convertToIds(tokenMaps: TokenMap[], maxLength = 0): TokenIds {
        return this.proxy.convertToIds(tokenMaps, maxLength) as TokenIds;
    }
}
