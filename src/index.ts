import * as bindings from 'bindings';

const vecxx = bindings('vecxx');
const {
    BPEVocab: VocabBinding,
    VocabVectorizer: VocabVectorizerBinding,
    VocabMapVectorizer: VocabMapVectorizerBinding
} = vecxx;

export type TokenTransform = (s: string) => string;
const identityTransform: TokenTransform = (s: string) => s;

export class BPEVocab {
    public readonly binding: any;

    constructor(vocabFile: string, codesFile: string) {
        this.binding = new VocabBinding(vocabFile, codesFile);
    }

    public lookup(token: string, transform?: TokenTransform): number {
        return this.binding.lookup(token, transform ?? identityTransform);
    }
}

export type Token = string;
export type Tokens = Token[];
export type TokenIds = { ids: number[]; size: number };

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

    constructor(private readonly vocab: BPEVocab, private readonly options?: VocabVectorizerOptions) {
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
