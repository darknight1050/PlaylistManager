#pragma once
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#define DECLARE_JSON_CLASS(namespaze, name, impl) \
namespace namespaze { \
    class name { \
    public: \
        void Deserialize(const rapidjson::Value& jsonValue); \
    impl \
    }; \
} \

#define GETTER_VALUE(type, name) \
private: \
    type name; \
public: \
    const type& Get##name() const { return name; }

#define GETTER_VALUE_OPTIONAL(type, name) \
private: \
    std::optional<type> name; \
public: \
    std::optional<type> Get##name() const { return name; }

#define DESERIALIZE_METHOD(namespaze, name, impl) \
void namespaze::name::Deserialize(const rapidjson::Value& jsonValue) { \
    impl \
}

#define DESERIALIZE_VALUE(name, jsonName, type) \
if (!jsonValue.HasMember(#jsonName)) throw #jsonName " not found"; \
if (!jsonValue[#jsonName].Is##type()) throw #jsonName ", type expected was: " #type; \
name = jsonValue[#jsonName].Get##type();

#define DESERIALIZE_VALUE_OPTIONAL(name, jsonName, type) \
if(jsonValue.HasMember(#jsonName) && jsonValue[#jsonName].Is##type()) { \
    name = jsonValue[#jsonName].Get##type(); \
} else { \
    name = std::nullopt; \
}

#define DESERIALIZE_CLASS(name, jsonName) \
if (!jsonValue.HasMember(#jsonName)) throw #jsonName " not found"; \
if (!jsonValue[#jsonName].IsObject()) throw #jsonName ", type expected was: JsonObject"; \
name.Deserialize(jsonValue[#jsonName]);

#define DESERIALIZE_VECTOR(name, jsonName, type) \
if (!jsonValue.HasMember(#jsonName)) throw #jsonName " not found"; \
name.clear(); \
auto& jsonName = jsonValue[#jsonName]; \
if(jsonName.IsArray()) { \
    for (auto it = jsonName.Begin(); it != jsonName.End(); ++it) { \
        type value; \
        value.Deserialize(*it); \
        name.push_back(value); \
    } \
} else throw #jsonName ", type expected was: JsonArray";
