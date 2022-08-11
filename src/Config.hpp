#pragma once

#include <string>
#include <vector>

namespace Dragon {

    enum class EntryType {
        String,
        List,
        Compound
    };
    struct ConfigEntry {
        std::string key;
        EntryType type;
        virtual ~ConfigEntry() {}
    };

    struct StringEntry : ConfigEntry {
        std::string value;
        StringEntry() {
            this->type = EntryType::String;
        }
        std::string getValue() {
            return this->value;
        }
    };

    struct ListEntry : ConfigEntry {
        std::vector<std::string> value;
        ListEntry() {
            this->type = EntryType::List;
        }
        std::string get(unsigned long index) {
            if (index >= this->value.size()) {
                std::cerr << "Index out of bounds" << std::endl;
                exit(1);
            }
            return this->value[index];
        }
        unsigned long size() {
            return this->value.size();
        }
    };

    struct CompoundEntry : ConfigEntry {
        std::vector<ConfigEntry> value;
        std::vector<CompoundEntry> compounds;
        std::vector<ListEntry> lists;
        std::vector<StringEntry> strings;
        CompoundEntry() {
            this->type = EntryType::Compound;
        }
        StringEntry getString(const std::string& key) {
            for (auto& entry : this->strings) {
                if (entry.key == key) {
                    return entry;
                }
            }
            std::cerr << "Could not find string entry with key " << key << std::endl;
            exit(1);
        }
        StringEntry getStringOrDefault(const std::string& key, const std::string& defaultValue) {
            for (auto& entry : this->strings) {
                if (entry.key == key) {
                    return entry;
                }
            }
            StringEntry entry;
            entry.key = key;
            entry.value = defaultValue;
            this->strings.push_back(entry);
            return entry;
        }
        ListEntry getList(const std::string& key) {
            for (auto& entry : this->lists) {
                if (entry.key == key) {
                    return entry;
                }
            }
            std::cerr << "Could not find list entry with key " << key << std::endl;
            exit(1);
        }
        CompoundEntry getCompound(const std::string& key) {
            for (auto& entry : this->compounds) {
                if (entry.key == key) {
                    return entry;
                }
            }
            std::cerr << "Could not find compound entry with key " << key << std::endl;
            exit(1);
        }
        void setString(const std::string& key, const std::string& value) {
            for (auto& entry : this->strings) {
                if (entry.key == key) {
                    entry.value = value;
                    return;
                }
            }
            StringEntry newEntry;
            newEntry.key = key;
            newEntry.value = value;
            this->strings.push_back(newEntry);
        }
    };

    struct Config {
        std::vector<ConfigEntry> entries;
        std::vector<CompoundEntry> compounds;
        std::vector<ListEntry> lists;
        std::vector<StringEntry> strings;

        CompoundEntry getCompound(const std::string& key) {
            for (auto& entry : this->compounds) {
                if (entry.key == key) {
                    return entry;
                }
            }
            std::cerr << "Could not find compound entry with key " << key << std::endl;
            exit(1);
        }
        ListEntry getList(const std::string& key) {
            for (auto& entry : this->lists) {
                if (entry.key == key) {
                    return entry;
                }
            }
            std::cerr << "Could not find list entry with key " << key << std::endl;
            exit(1);
        }
        StringEntry getString(const std::string& key) {
            for (auto& entry : this->strings) {
                if (entry.key == key) {
                    return entry;
                }
            }
            std::cerr << "Could not find string entry with key " << key << std::endl;
            exit(1);
        }
        StringEntry getStringOrDefault(const std::string& key, const std::string& defaultValue) {
            for (auto& entry : this->strings) {
                if (entry.key == key) {
                    return entry;
                }
            }
            StringEntry entry;
            entry.key = key;
            entry.value = defaultValue;
            this->strings.push_back(entry);
            return entry;
        }
        void setString(const std::string& key, const std::string& value) {
            for (auto& entry : this->strings) {
                if (entry.key == key) {
                    entry.value = value;
                    return;
                }
            }
            StringEntry newEntry;
            newEntry.key = key;
            newEntry.value = value;
            this->strings.push_back(newEntry);
        }

        void parse(const std::string& config) {
            int configSize = config.size();
            bool inStr = false;
            std::string data;
            int z = 0;
            for (int *i = &z; (*i) < configSize; ++(*i)) {
                if (!inStr && config.at((*i)) == ' ') continue;
                if (config.at((*i)) == '"' && config.at((*i) - 1) != '\\') {
                    inStr = !inStr;
                    continue;
                }
                if (config.at((*i)) == '\n') {
                    inStr = false;
                    continue;
                }
                char c = config.at((*i));
                data += c;
            }
            int dataSize = data.size();
            std::string str;

            z = 0;
            for (int (*i) = &z; (*i) < dataSize; (*i)++) {
                char c = data.at((*i));

                if (c == ';') {
                    str.clear();
                    continue;
                }

                if (c == ':') {
                    std::string key = str;
                    str.clear();
                    c = data.at(++(*i));
                    if (c == '[') {
                        std::vector<std::string> values;
                        c = data.at(++(*i));
                        while (c != ']') {
                            std::string next;
                            while (c != ';') {
                                next += c;
                                c = data.at(++(*i));
                                if (c == ';') {
                                    values.push_back(next);
                                    break;
                                }
                            }
                            c = data.at(++(*i));
                        }
                        c = data.at(++(*i));
                        ListEntry entry;
                        entry.key = key;
                        entry.value = values;
                        entries.push_back(entry);
                        lists.push_back(entry);
                    } else if (c == '{') {
                        CompoundEntry entry = parseCompound(data, i);
                        entry.key = key;
                        entries.push_back(entry);
                        compounds.push_back(entry);
                    } else {
                        std::string value = "";
                        while (c != ';') {
                            value += c;
                            c = data.at(++(*i));
                        }
                        StringEntry entry;
                        entry.key = key;
                        entry.value = value;
                        entries.push_back(entry);
                        strings.push_back(entry);
                    }
                    str.clear();
                    continue;
                }

                str += c;
            }
        }

        CompoundEntry parseCompound(std::string& data, int* i) {
            CompoundEntry compound;
            char c = data.at(++(*i));
            while (c != '}') {
                std::string key = "";
                while (c != ':') {
                    key += c;
                    c = data.at(++(*i));
                }
                c = data.at(++(*i));
                if (c == '[') {
                    std::vector<std::string> values;
                    c = data.at(++(*i));
                    while (c != ']') {
                        std::string next;
                        while (c != ';') {
                            next += c;
                            c = data.at(++(*i));
                            if (c == ';') {
                                values.push_back(next);
                                break;
                            }
                        }
                        c = data.at(++(*i));
                    }
                    c = data.at(++(*i));
                    ListEntry entry;
                    entry.key = key;
                    entry.value = values;
                    entries.push_back(entry);
                    compound.lists.push_back(entry);
                } else if (c == '{') {
                    CompoundEntry entry = parseCompound(data, i);
                    entry.key = key;
                    entries.push_back(entry);
                    compound.compounds.push_back(entry);
                } else {
                    std::string value = "";
                    while (c != ';') {
                        value += c;
                        c = data.at(++(*i));
                    }
                    StringEntry entry;
                    entry.key = key;
                    entry.value = value;
                    entries.push_back(entry);
                    compound.strings.push_back(entry);
                }
                c = data.at(++(*i));
            }
            c = data.at(++(*i));
            return compound;
        }
    };
}
