#include <regex>

#if defined(_WIN32)
#define OS_NAME "windows"
#elif defined(__APPLE__)
#define OS_NAME "macos"
#elif defined(__linux__)
#define OS_NAME "linux"
#else
#define OS_NAME "unknown"
#endif

#include "DragonConfig.hpp"

using namespace std;
using namespace DragonConfig;

#define DRAGON_LOG std::cout << "[Dragon] "
#define DRAGON_ERR std::cerr << "[Dragon] "

static CompoundEntry* currentParsingRoot = nullptr;

static std::string replaceAll(std::string src, std::string from, std::string to) {
    try {
        return regex_replace(src, std::regex(from), to);
    } catch (std::regex_error& e) {
        return src;
    }
}

std::string ConfigEntry::getKey() const { return key; }
EntryType ConfigEntry::getType() const { return type; }
void ConfigEntry::setKey(const std::string& key) { this->key = key; }
void ConfigEntry::setType(EntryType type) { this->type = type; }
void ConfigEntry::print(std::ostream& out, int indent) { (void) out; (void) indent; }

StringEntry::StringEntry() {
    this->setType(EntryType::String);
}
std::string StringEntry::getValue() {
    return this->value;
}
void StringEntry::setValue(std::string value) {
    size_t index;
    std::string tmp = value;
    std::string newVal = value;
    while ((index = tmp.find("$(")) != std::string::npos) {
        size_t endIndex = tmp.find_first_of(")");
        auto s = tmp.substr(index + 2, endIndex - index - 2);
        tmp = tmp.substr(endIndex + 1);
        StringEntry* key = currentParsingRoot->getStringByPath(s);
        if (key == nullptr) {
            DRAGON_ERR << "Could not resolve path '" << s << "' for macro. Maybe that key doesn't exist yet?";
            return;
        }
        newVal = replaceAll(newVal, "\\$\\(" + s + "\\)", key->getValue());
    }
    this->value = newVal;
}
bool StringEntry::isEmpty() {
    return this->value.empty();
}
bool StringEntry::operator==(const StringEntry& other) {
    return (this->value == other.value && this->getKey() == other.getKey());
}
bool StringEntry::operator!=(const StringEntry& other) {
    return !operator==(other);
}
void StringEntry::print(std::ostream& stream, int indent) {
    stream << std::string(indent, ' ');
    if (this->getKey().size()) {
        stream << this->getKey() << ": ";
    }
    stream << "\"" << this->value << "\";" << std::endl;
}

ListEntry::ListEntry() {
    this->setType(EntryType::List);
    this->value = std::vector<ConfigEntry*>();
}
ConfigEntry* ListEntry::get(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        return nullptr;
    }
    return this->value[index];
}
StringEntry* ListEntry::getString(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        return nullptr;
    }
    return (this->value[index]->getType() == EntryType::String ? reinterpret_cast<StringEntry*>(this->value[index]) : nullptr);
}
CompoundEntry* ListEntry::getCompound(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        return nullptr;
    }
    return (this->value[index]->getType() == EntryType::Compound ? reinterpret_cast<CompoundEntry*>(this->value[index]) : nullptr);
}
ListEntry* ListEntry::getList(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        return nullptr;
    }
    return (this->value[index]->getType() == EntryType::List ? reinterpret_cast<ListEntry*>(this->value[index]) : nullptr);
}
unsigned long ListEntry::size() {
    return this->value.size();
}
void ListEntry::add(ConfigEntry* value) {
    this->value.push_back(value);
}
void ListEntry::addAll(std::vector<ConfigEntry*> values) {
    this->value.insert(this->value.end(), values.begin(), values.end());
}
void ListEntry::remove(unsigned long index) {
    if (index >= this->value.size()) {
        std::cerr << "Index out of bounds" << std::endl;
        return;
    }
    this->value.erase(this->value.begin() + index);
}
void ListEntry::removeAll(std::vector<unsigned long> indices) {
    for (unsigned long i = 0; i < indices.size(); i++) {
        this->remove(indices[i]);
    }
}
void ListEntry::clear() {
    this->value.clear();
}
bool ListEntry::isEmpty() {
    return this->value.empty();
}
bool ListEntry::operator==(const ListEntry& other) {
    return (this->value == other.value && this->getKey() == other.getKey());
}
bool ListEntry::operator!=(const ListEntry& other) {
    return !operator==(other);
}
void ListEntry::print(std::ostream& stream, int indent) {
    stream << std::string(indent, ' ');
    if (this->getKey().size()) {
        stream << this->getKey() << ": ";
    }
    stream << "[" << std::endl;
    indent += 2;
    for (unsigned long i = 0; i < this->value.size(); i++) {
        this->value[i]->print(stream, indent);
    }
    indent -= 2;
    stream << std::string(indent, ' ') << "];" << std::endl;
}

CompoundEntry::CompoundEntry() {
    this->setType(EntryType::Compound);
}
bool CompoundEntry::hasMember(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getKey() == key) {
            return true;
        }
    }
    return false;
}
StringEntry* CompoundEntry::getString(const std::string& key) {
    for (auto& entry : this->entries) {
        if (entry->getType() == EntryType::String && entry->getKey() == key) {
            return reinterpret_cast<StringEntry*>(entry);
        }
    }
    return nullptr;
}
StringEntry* CompoundEntry::getStringOrDefault(const std::string& key, const std::string& defaultValue) {
    for (auto entry : this->entries) {
        if (entry->getType() == EntryType::String && entry->getKey() == key) {
            return reinterpret_cast<StringEntry*>(entry);
        }
    }
    StringEntry* entry = new StringEntry();
    entry->setValue(defaultValue);
    return entry;
}
ListEntry* CompoundEntry::getList(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getType() == EntryType::List && entry->getKey() == key) {
            return reinterpret_cast<ListEntry*>(entry);
        }
    }
    return nullptr;
}
CompoundEntry* CompoundEntry::getCompound(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getType() == EntryType::Compound && entry->getKey() == key) {
            return reinterpret_cast<CompoundEntry*>(entry);
        }
    }
    return nullptr;
}
StringEntry* CompoundEntry::getStringByPath(const std::string& path) {
    ConfigEntry* entry = this->resolvePath(path);
    if (!entry || entry->getType() != EntryType::String) {
        return nullptr;
    }
    return reinterpret_cast<StringEntry*>(entry);
}
StringEntry* CompoundEntry::getStringOrDefaultByPath(const std::string& path, const std::string& defaultValue) {
    ConfigEntry* entry = this->resolvePath(path);
    if (!entry || entry->getType() != EntryType::String) {
        StringEntry* entry = new StringEntry();
        entry->setValue(defaultValue);
        return entry;
    }
    return reinterpret_cast<StringEntry*>(entry);
}
ListEntry* CompoundEntry::getListByPath(const std::string& path) {
    ConfigEntry* entry = this->resolvePath(path);
    if (!entry || entry->getType() != EntryType::List) {
        return nullptr;
    }
    return reinterpret_cast<ListEntry*>(entry);
}
CompoundEntry* CompoundEntry::getCompoundByPath(const std::string& path) {
    ConfigEntry* entry = this->resolvePath(path);
    if (!entry || entry->getType() != EntryType::Compound) {
        return nullptr;
    }
    return reinterpret_cast<CompoundEntry*>(entry);
}

static std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> body;
    size_t start = 0;
    size_t end = 0;
    while ((end = str.find(delimiter, start)) != std::string::npos)
    {
        body.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
    }
    body.push_back(str.substr(start));
    return body;
}

ConfigEntry* CompoundEntry::get(const std::string& key) {
    for (auto entry : this->entries) {
        if (entry->getKey() == key) {
            return entry;
        }
    }
    return nullptr;
}

ConfigEntry* CompoundEntry::resolvePath(const std::string& path) {
    std::string internalCopy = path;
    auto pathAsVec = split(internalCopy, ".");
    CompoundEntry* current = this;
    size_t i = 0;
    if (pathAsVec.size() > 1) {
        for (; i < pathAsVec.size() - 1; i++) {
            current = current->getCompound(pathAsVec[i]);
        }
    }
    return current->get(pathAsVec[i]);
}
void CompoundEntry::setString(const std::string& key, const std::string& value) {
    for (auto& entry : this->entries) {
        if (entry->getType() ==  EntryType::String && entry->getKey() == key) {
            reinterpret_cast<StringEntry*>(entry)->getValue() = value;
            return;
        }
    }
    StringEntry* newEntry = new StringEntry();
    newEntry->setKey(key);
    newEntry->setValue(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addString(const std::string& key, const std::string& value) {
    if (this->hasMember(key)) {
        std::cerr << "String with key '" << key << "' already exists" << std::endl;
        return;
    }
    StringEntry* newEntry = new StringEntry();
    newEntry->setKey(key);
    newEntry->setValue(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addList(const std::string& key, const std::vector<ConfigEntry*>& value) {
    if (this->hasMember(key)) {
        std::cerr << "List with key '" << key << "' already exists!" << std::endl;
        return;
    }
    ListEntry* newEntry = new ListEntry();
    newEntry->setKey(key);
    newEntry->addAll(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addList(const std::string& key, ConfigEntry* value) {
    if (this->hasMember(key)) {
        std::cerr << "List with key '" << key << "' already exists!" << std::endl;
        return;
    }
    ListEntry* newEntry = new ListEntry();
    newEntry->setKey(key);
    newEntry->add(value);
    this->entries.push_back(newEntry);
}
void CompoundEntry::addList(ListEntry* value) {
    if (this->hasMember(value->getKey())) {
        std::cerr << "List with key '" << value->getKey() << "' already exists!" << std::endl;
        return;
    }
    this->entries.push_back(reinterpret_cast<ConfigEntry*>(value));
}
void CompoundEntry::addCompound(CompoundEntry* value) {
    if (this->hasMember(value->getKey())) {
        std::cerr << "Compound with key '" << value->getKey() << "' already exists!" << std::endl;
        return;
    }
    this->entries.push_back(reinterpret_cast<ConfigEntry*>(value));
}
void CompoundEntry::remove(const std::string& key) {
    for (u_long i = 0; i < this->entries.size(); i++) {
        if (this->entries[i]->getKey() == key) {
            this->entries[i] = nullptr;
            return;
        }
    }
}
void CompoundEntry::removeAll() {
    this->entries.clear();
}
bool CompoundEntry::isEmpty() {
    return this->entries.empty();
}
void CompoundEntry::print(std::ostream& stream, int indent) {
    if (this->getKey() == ".root") {
        for (u_long i = 0; i < this->entries.size(); i++) {
            this->entries[i]->print(stream, indent);
        }
    } else {
        stream << std::string(indent, ' ');
        if (this->getKey().size()) {
            stream << this->getKey() << ": ";
        } 
        stream << "{" << std::endl;
        indent += 2;
        for (u_long i = 0; i < this->entries.size(); i++) {
            this->entries[i]->print(stream, indent);
        }
        indent -= 2;
        stream << std::string(indent, ' ') << "};" << std::endl;
    }
}
CompoundEntry* ConfigParser::parse(const std::string& configFile) {
    FILE* fp = fopen(configFile.c_str(), "r");
    if (!fp) {
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buf = new char[size + 1];
    fread(buf, 1, size, fp);
    buf[size] = '\0';
    fclose(fp);
    std::string config;
    for (size_t i = 0; buf[i]; i++) {
        if (buf[i] == '#') {
            while (buf[i] != '\n' && buf[i] != '\0') {
                i++;
            }
        }
        config += buf[i];
    }
    delete[] buf;

    int configSize = config.size();
    bool inStr = false;
    std::string data;
    int z = 0;
    for (int *i = &z; (*i) < configSize; ++(*i)) {
        if (!inStr && config.at((*i)) == ' ') continue;
        if (config.at((*i)) == '"' && config.at((*i) - 1) != '\\') {
            inStr = !inStr;
            char c = config.at((*i));
            data += c;
            continue;
        }
        if (config.at((*i)) == '\n') {
            inStr = false;
            continue;
        }
        char c = config.at((*i));
        data += c;
    }
    std::string str;

    data = ".root:{" + data + "};";
    std::string key = ".root";
    int i = 6;
    CompoundEntry* rootEntry = parseCompound(data, &i);
    rootEntry->setKey(".root");
    return rootEntry;
}

bool ConfigParser::isValidIdentifier(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-';
}

ListEntry* ConfigParser::parseList(std::string& data, int* i) {
    ListEntry* list = new ListEntry();
    char c = data.at(++(*i));
    while (c != ']') {
        c = data.at((*i));
        if (c == '[') {
            list->add(this->parseList(data, i));
        } else if (c == '{') {
            list->add(this->parseCompound(data, i));
        } else {
            list->add(this->parseString(data, i));
        }
        c = data.at(++(*i));
    }
    c = data.at(++(*i));
    if (c != ';') {
        DRAGON_ERR << "Missing semicolon" << std::endl;
        return nullptr;
    }
    return list;
}

StringEntry* ConfigParser::parseString(std::string& data, int* i) {
    std::string value = "";
    char c = data.at(++(*i));
    bool escaped = false;
    while (true) {
        if (c == '"' && !escaped) break;
        if (c == '\\') {
            escaped = !escaped;
        } else {
            escaped = false;
        }
        value += c;
        c = data.at(++(*i));
    }
    c = data.at(++(*i));
    if (c != ';') {
        DRAGON_ERR << "Invalid value: '" << value << "'" << std::endl;
        return nullptr;
    }
    StringEntry* entry = new StringEntry();
    entry->setValue(value);
    return entry;
}

CompoundEntry* ConfigParser::parseCompound(std::string& data, int* i) {
    CompoundEntry* compound = new CompoundEntry();
    bool resetCurrentAfter = false;
    if (currentParsingRoot == nullptr) {
        currentParsingRoot = compound;
        resetCurrentAfter = true;
    }
    char c = data.at(++(*i));
    bool lastConditionWasTrue = false;
    while (c != '}') {
        std::string key = "";
        for (; isValidIdentifier(c); c = data.at(++(*i))) {
            key += c;
        }
        c = data.at(++(*i));
        if (c == '[') {
            ListEntry* entry = this->parseList(data, i);
            if (!entry) {
                if (resetCurrentAfter)
                    currentParsingRoot = nullptr;
                return nullptr;
            }
            entry->setKey(key);
            compound->entries.push_back(entry);
        } else if (c == '{') {
            CompoundEntry* entry = parseCompound(data, i);
            if (!entry) {
                if (resetCurrentAfter)
                    currentParsingRoot = nullptr;
                return nullptr;
            }
            entry->setKey(key);
            compound->entries.push_back(entry);
        } else if (c == '"') {
            StringEntry* entry = this->parseString(data, i);
            if (!entry) {
                if (resetCurrentAfter)
                    currentParsingRoot = nullptr;
                return nullptr;
            }
            entry->setKey(key);
            compound->entries.push_back(entry);
        } else {
            const std::string& str = data.substr(*i);
            auto strstarts = [](const std::string& str, const std::string& prefix) {
                return str.substr(0, prefix.size()) == prefix;
            };
            if (strstarts(str, "if<")) {
                *i += 3;
                std::string what = data.substr(*i, data.find_first_of('>') - *i);
                *i += what.size();
                if (data.at(++(*i)) != '(') {
                    DRAGON_ERR << "Invalid if<?> statement: expected '(' after 'if<?>'" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                std::string condition = data.substr(++(*i), data.find_first_of(')') - *i);
                *i += condition.size() + 1;
                if (data.at(*i) != '{') {
                    DRAGON_ERR << "Invalid if<?> statement: expected '{' after condition" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                *i += 1;
                ConfigEntry* entry = nullptr;
                if (data.at(*i) == '{') {
                    entry = this->parseCompound(data, i);
                } else if (data.at(*i) == '[') {
                    entry = this->parseList(data, i);
                } else if (data.at(*i) == '"') {
                    entry = this->parseString(data, i);
                } else {
                    DRAGON_ERR << "Invalid if<?> statement: expected '{', '[' or '\"' after '{'" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                auto evaluate = [](const std::string& what, const std::string& is) {
                    if (what == "os") {
                        return is == OS_NAME;
                    } else {
                        DRAGON_ERR << "Invalid if<?> statement: unknown identifier '" << what << "'" << std::endl;
                        return false;
                    }
                };
                if (!entry) {
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                if (evaluate(what, condition)) {
                    entry->setKey(key);
                    compound->entries.push_back(entry);
                    lastConditionWasTrue = true;
                }
                if (data.at(*i) != ';') {
                    DRAGON_ERR << "Invalid if<?> statement: expected ';' after entry" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                *i += 2;

                const std::string& elseStatement = data.substr(*i);
                if (!strstarts(elseStatement, "else{")) {
                    DRAGON_ERR << "Invalid if<?> statement: expected 'else' after entry" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                *i += 4;
                if (data.at(*i) != '{') {
                    DRAGON_ERR << "Invalid else statement: expected '{' after 'else'" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                *i += 1;
                entry = nullptr;
                if (data.at(*i) == '{') {
                    entry = this->parseCompound(data, i);
                } else if (data.at(*i) == '[') {
                    entry = this->parseList(data, i);
                } else if (data.at(*i) == '"') {
                    entry = this->parseString(data, i);
                } else {
                    DRAGON_ERR << "Invalid else statement: expected '{', '[' or '\"' after '{'" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                if (!entry) {
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                if (!lastConditionWasTrue) {
                    entry->setKey(key);
                    compound->entries.push_back(entry);
                }
                lastConditionWasTrue = false;
                if (data.at(*i) != ';') {
                    DRAGON_ERR << "Invalid else statement: expected ';' after entry" << std::endl;
                    if (resetCurrentAfter)
                        currentParsingRoot = nullptr;
                    return nullptr;
                }
                *i += 2;
            } else {
                DRAGON_ERR << "Invalid entry: '" << key << "'" << std::endl;
                if (resetCurrentAfter)
                    currentParsingRoot = nullptr;
                return nullptr;
            }
        }
        c = data.at(++(*i));
    }
    c = data.at(++(*i));
    if (c != ';') {
        DRAGON_ERR << "Invalid compound entry!" << std::endl;
        if (resetCurrentAfter)
            currentParsingRoot = nullptr;
        return nullptr;
    }
    if (resetCurrentAfter)
        currentParsingRoot = nullptr;
    return compound;
}
