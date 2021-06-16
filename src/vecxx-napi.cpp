#include <napi.h>
#include <string>
#include "vecxx/vecxx.h"

class TransformWrapper {
public:
    TransformWrapper(const Napi::FunctionReference& ref, const Napi::Env& env) : ref(ref), env(env) {}
    std::string transform(std::string v) {
        Napi::HandleScope scope(this->env);
        std::vector<napi_value> args = { Napi::String::New(this->env, v) };
        return (std::string)this->ref.Call(this->env.Null(), args).ToString();
    }

private:
    const Napi::FunctionReference& ref;
    const Napi::Env& env;
};

TokenList_T toTokenList(const Napi::Array &arr) {
    auto length = arr.Length();
    TokenList_T vec(length);
    for (auto i = 0; i < length; ++i) {
        vec[i] = static_cast<Napi::Value>(arr[i]).ToString();
    }
    return vec;
}

TokenMapList_T toTokenMapList(const Napi::Array &arr) {
    auto length = arr.Length();
    TokenMapList_T vec(length);
    for (auto i = 0; i < length; ++i) {
        Napi::Object v = static_cast<Napi::Value>(arr[i]).ToObject();
        Napi::Array props = v.GetPropertyNames();
        auto propsLength = props.Length();
        TokenMap_T map;
        for (auto j = 0; j < propsLength; ++j) {
            std::string key = static_cast<Napi::Value>(props[j]).ToString();
            map[key] = (std::string)v.Get(key).ToString();
        }
        vec[i] = map;
    }
    return vec;
}

Napi::Array fromTokenList(const TokenList_T &vec, const Napi::Env &env) {
    Napi::Array arr = Napi::Array::New(env, vec.size());
    int i = 0;
    for (auto it = std::begin(vec); it != std::end(vec); ++it) {
        Napi::HandleScope scope(env);
        arr[i++] = Napi::String::New(env, *it);
    }
    return arr;
}

Napi::Array fromIdsList(const VecList_T &vec, const Napi::Env &env) {
    Napi::Array arr = Napi::Array::New(env, vec.size());
    int i = 0;
    for (auto it = std::begin(vec); it != std::end(vec); ++it) {
        Napi::HandleScope scope(env);
        arr[i++] = Napi::Number::New(env, *it);
    }
    return arr;
}


class VocabWrapper : public Napi::ObjectWrap<VocabWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    VocabWrapper(const Napi::CallbackInfo &info);
    ~VocabWrapper();
    Vocab *getValue() { return this->value; }
private:
    Vocab *value = NULL;
    Napi::Value lookup(const Napi::CallbackInfo &info);
};

Napi::Object VocabWrapper::Init(Napi::Env env, Napi::Object exports) {
    exports.Set("Vocab", DefineClass(env, "Vocab", {
            InstanceMethod<&VocabWrapper::lookup>("lookup"),
    }));
    return exports;
}

VocabWrapper::VocabWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<VocabWrapper>(info) {
    if (info.Length() < 2) {
        Napi::TypeError::New(info.Env(), "You must supply at least 2 arguments").ThrowAsJavaScriptException();
        return;
    }
    std::string vocabType = (std::string) info[0].ToString();
    if (vocabType == "word-file") {
        this->value = new WordVocab((std::string) info[1].ToString());
    } else if (vocabType == "word-tokens") {
        Napi::Reference<Napi::Array> tokenArray = Napi::Weak(info[1].As<Napi::Array>());
        TokenList_T tokens = toTokenList(tokenArray.Value());
        this->value = new WordVocab(tokens);
    } else if (vocabType == "word-counter") {
        Napi::Object counterRecord = info[1].As<Napi::Object>();
        Napi::Array fields = counterRecord.GetPropertyNames();

        auto numFields = fields.Length();
        Counter_T counter;
        for (auto i = 0; i < numFields; ++i) {
            std::string field = static_cast<Napi::Value>(fields[i]).ToString();
            counter[field] = (int)counterRecord.Get(field).ToNumber();
        }
        this->value = new WordVocab(counter);
    }
    else if (vocabType == "bpe") {
        if (info.Length() < 3) {
            Napi::TypeError::New(info.Env(), "You must supply 2 filenames to create BPEVocab").ThrowAsJavaScriptException();
            return;
        }
        this->value = new BPEVocab((std::string) info[1].ToString(), (std::string) info[2].ToString());
    } else {
        Napi::TypeError::New(info.Env(), "Invalid vocab type specified").ThrowAsJavaScriptException();
    }
}

VocabWrapper::~VocabWrapper() {
    if (this->value) {
        delete this->value;
    }
}

Napi::Value VocabWrapper::lookup(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    auto numArgs = info.Length();
    if (numArgs != 2) {
        Napi::TypeError::New(env, "Must supply 2 arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    std::string token = (std::string) info[0].ToString();
    Napi::FunctionReference func = Napi::Weak(info[1].As<Napi::Function>());
    const TransformWrapper transformer(func, env);
    Transform_T transform = std::bind(&TransformWrapper::transform, transformer, std::placeholders::_1);
    return Napi::Number::New(env, this->value->lookup(token, transform));
}


class VocabVectorizerWrapper : public Napi::ObjectWrap<VocabVectorizerWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    VocabVectorizerWrapper(const Napi::CallbackInfo &info);
    Napi::Value convertToPieces(const Napi::CallbackInfo &info);
    Napi::Value convertToIds(const Napi::CallbackInfo &info);
private:
    VocabWrapper* vocab;
    Napi::FunctionReference transform;
    Napi::Reference<Napi::Array> emitBeginToken;
    Napi::Reference<Napi::Array> emitEndToken;
};

Napi::Object VocabVectorizerWrapper::Init(Napi::Env env, Napi::Object exports) {
    exports.Set("VocabVectorizer", DefineClass(env, "VocabVectorizer", {
            InstanceMethod<&VocabVectorizerWrapper::convertToPieces>("convertToPieces"),
            InstanceMethod<&VocabVectorizerWrapper::convertToIds>("convertToIds"),
    }));
    return exports;
}

VocabVectorizerWrapper::VocabVectorizerWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<VocabVectorizerWrapper>(info) {
    if (info.Length() < 4) {
        Napi::TypeError::New(info.Env(), "You must supply 4 arguments").ThrowAsJavaScriptException();
        return;
    }
    vocab = Napi::ObjectWrap<VocabWrapper>::Unwrap(info[0].ToObject());
    transform = Napi::Persistent(info[1].As<Napi::Function>());
    emitBeginToken = Napi::Persistent(info[2].As<Napi::Array>());
    emitEndToken = Napi::Persistent(info[3].As<Napi::Array>());
}

Napi::Value VocabVectorizerWrapper::convertToPieces(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    auto numArgs = info.Length();
    if (numArgs != 1) {
        Napi::TypeError::New(env, "Must supply 1 argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    const TransformWrapper transformer(this->transform, env);
    Transform_T transformProxy = std::bind(&TransformWrapper::transform, transformer, std::placeholders::_1);
    TokenList_T beginTokens = toTokenList(this->emitBeginToken.Value());
    TokenList_T endTokens = toTokenList(this->emitEndToken.Value());
    VocabVectorizer vec(this->vocab->getValue(), transformProxy, beginTokens, endTokens);

    TokenList_T tokens = toTokenList(info[0].As<Napi::Array>());
    TokenList_T pieces = vec.convert_to_pieces(tokens);
    return fromTokenList(pieces, env);
}

Napi::Value VocabVectorizerWrapper::convertToIds(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    auto numArgs = info.Length();
    if (numArgs != 2) {
        Napi::TypeError::New(env, "Must supply 2 arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    const TransformWrapper transformer(this->transform, env);
    Transform_T transformProxy = std::bind(&TransformWrapper::transform, transformer, std::placeholders::_1);
    TokenList_T beginTokens = toTokenList(this->emitBeginToken.Value());
    TokenList_T endTokens = toTokenList(this->emitEndToken.Value());
    VocabVectorizer vec(this->vocab->getValue(), transformProxy, beginTokens, endTokens);

    TokenList_T tokens = toTokenList(info[0].As<Napi::Array>());
    auto maxLength = info[1].As<Napi::Number>();
    std::tuple<VecList_T, long unsigned int> result = vec.convert_to_ids(tokens, maxLength.Int64Value());

    Napi::Object obj = Napi::Object::New(env);
    obj.Set("ids", fromIdsList(std::get<0>(result), env));
    obj.Set("size", Napi::Number::New(env, std::get<1>(result)));
    return obj;
}

class VocabMapVectorizerWrapper : public Napi::ObjectWrap<VocabMapVectorizerWrapper> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    VocabMapVectorizerWrapper(const Napi::CallbackInfo &info);
    Napi::Value convertToPieces(const Napi::CallbackInfo &info);
    Napi::Value convertToIds(const Napi::CallbackInfo &info);
private:
    VocabWrapper* vocab;
    Napi::FunctionReference transform;
    Napi::Reference<Napi::Array> emitBeginToken;
    Napi::Reference<Napi::Array> emitEndToken;
    Napi::Reference<Napi::Array> fields;
    std::string delim;
};


Napi::Object VocabMapVectorizerWrapper::Init(Napi::Env env, Napi::Object exports) {
    exports.Set("VocabMapVectorizer", DefineClass(env, "VocabMapVectorizer", {
            InstanceMethod<&VocabMapVectorizerWrapper::convertToPieces>("convertToPieces"),
            InstanceMethod<&VocabMapVectorizerWrapper::convertToIds>("convertToIds"),
    }));
    return exports;
}

VocabMapVectorizerWrapper::VocabMapVectorizerWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<VocabMapVectorizerWrapper>(info) {
    if (info.Length() < 6) {
        Napi::TypeError::New(info.Env(), "You must supply 6 arguments").ThrowAsJavaScriptException();
        return;
    }
    vocab = Napi::ObjectWrap<VocabWrapper>::Unwrap(info[0].ToObject());
    transform = Napi::Persistent(info[1].As<Napi::Function>());
    emitBeginToken = Napi::Persistent(info[2].As<Napi::Array>());
    emitEndToken = Napi::Persistent(info[3].As<Napi::Array>());
    fields = Napi::Persistent(info[4].As<Napi::Array>());
    delim = (std::string)info[5].ToString();
}

Napi::Value VocabMapVectorizerWrapper::convertToPieces(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    auto numArgs = info.Length();
    if (numArgs != 1) {
        Napi::TypeError::New(env, "Must supply 1 argument").ThrowAsJavaScriptException();
        return env.Null();
    }

    const TransformWrapper transformer(this->transform, env);
    Transform_T transformProxy = std::bind(&TransformWrapper::transform, transformer, std::placeholders::_1);
    TokenList_T beginTokens = toTokenList(this->emitBeginToken.Value());
    TokenList_T endTokens = toTokenList(this->emitEndToken.Value());
    VocabMapVectorizer vec(this->vocab->getValue(), transformProxy, beginTokens, endTokens);

    TokenMapList_T tokenMap = toTokenMapList(info[0].As<Napi::Array>());
    TokenList_T pieces = vec.convert_to_pieces(tokenMap);
    return fromTokenList(pieces, env);
}

Napi::Value VocabMapVectorizerWrapper::convertToIds(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    auto numArgs = info.Length();
    if (numArgs != 2) {
        Napi::TypeError::New(env, "Must supply 2 arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    const TransformWrapper transformer(this->transform, env);
    Transform_T transformProxy = std::bind(&TransformWrapper::transform, transformer, std::placeholders::_1);
    TokenList_T beginTokens = toTokenList(this->emitBeginToken.Value());
    TokenList_T endTokens = toTokenList(this->emitEndToken.Value());
    VocabMapVectorizer vec(this->vocab->getValue(), transformProxy, beginTokens, endTokens);

    TokenMapList_T tokenMaps = toTokenMapList(info[0].As<Napi::Array>());
    auto maxLength = info[1].As<Napi::Number>();
    std::tuple<VecList_T, long unsigned int> result = vec.convert_to_ids(tokenMaps, maxLength.Int64Value());

    Napi::Object obj = Napi::Object::New(env);
    obj.Set("ids", fromIdsList(std::get<0>(result), env));
    obj.Set("size", Napi::Number::New(env, std::get<1>(result)));
    return obj;
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    VocabWrapper::Init(env, exports);
    VocabVectorizerWrapper::Init(env, exports);
    VocabMapVectorizerWrapper::Init(env, exports);
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
