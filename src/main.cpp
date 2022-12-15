#include <iostream>
#include <fstream>
#include <filesystem>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include "md5.h"

namespace fs = std::filesystem;
using namespace std;

bool force  = false; // ignore onchange 
int height  = 0;     // height in additional target

bool   changed(string fpath);
string variable(json& js, string name);
string parse(json& js, string cmd);

namespace show {
    void info() {
        cout << "Build made by Alepacho, 2022" << endl;
        cout << "Usage: ./build [options] target..." << endl;
        cout << "Options:" << endl;
        cout << "   " << left << setw(16) << "-help"    << left << setw(6) << " Show info." << endl;
        cout << "   " << left << setw(16) << "-targets" << left << setw(6) << " Show all available targets." << endl;
        cout << "   " << left << setw(16) << "-force"   << left << setw(6) << " Ignore 'onchange' parameter for next target." << endl;
    }

    void targets(json& js) {
        cout << "Available targets:" << endl;
        for (auto& t : js["targets"]) {
            string desc;
            if (!t["description"].is_null()) desc = t["description"]; 
            cout << "   " << left << setw(11) << (string)t["name"] << left << setw(6) << " " << desc << endl;
        }
    }
}

string get_file_data(string filepath) {
    ifstream in(filepath);
    if (!in.is_open()) {
        cerr << "build::error: Unable to open '" << filepath << "' file!" << endl;
        return "";
    }

    string fdata((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    in.close();

    return fdata;
}

// check hash
bool changed(string fpath) {
    if (!fs::exists(".build")) fs::create_directory(".build");
    ifstream in(".build/changes.json");

    json js;
    if (in.is_open()) {
        try {
            in >> js;
        } catch (json::parse_error& ex) {
            cout << "err" << endl;
            js["hashes"] = {};
        }
        in.close();
    } else {
        json js;
        js["hashes"] = {};
    }
    ofstream out(".build/changes.json");

    if (!out.is_open()) {
        cerr << "build::warning: unable to open '" << ".build/changes.json" << "' file!" << endl;
        return true;
    }

    string fdata = get_file_data(fpath);
    if (fdata.length() == 0) return true;

    MD5 md5;
    string hash = md5(fdata);

    auto& js_hash = js["hashes"][fpath];
    if (!js_hash.is_string()) {
        js_hash = (string)hash;
    } else {
        if ((string)js_hash != (string)hash) {
            js_hash = (string)hash;
        } else {
            out << setw(4) << js << endl;
            out.close();
            return false;
        }
    }

    out << setw(4) << js << endl;
    out.close();

    return true;
}

string variable(json& js, string name) {
    if (js["variables"].empty()) {
        return "";
    }

    auto var = js["variables"][name];
    if (!var.is_null()) {
        switch (var.type()) {
            case nlohmann::detail::value_t::string: {
                return parse(js, var);
            } break;
            default:
                return "";
        }
    } else {
        cerr << "build::warning: unable to find variable '" << name << "'" << endl;
    }

    return "";
}

string parse(json& js, string cmd) {
    string result = cmd;

    size_t b = result.find("${");
    size_t e = b;
    while (b != string::npos) {
        while (result.at(e) != '}' && e < result.length()) e++;
        string var = result.substr(b + 2, (e - b) - 2);

        result.replace(b, e - b + 1, variable(js, var));

        b = result.find("${"); e = b;
    }
    
    return result;
}

bool execute(json& js, string name = "all") {
    auto targets = js["targets"];
    string result;
    for (auto& t : targets) {
        if (t["name"] == name) {
            height++;
            if (t["force"] == true) {
                force = true;
            }

            if (!t["targets"].empty()) {
                for (auto& tt : t["targets"]) {
                    if (!tt.is_string()) {
                        cerr << "build::error: data in additional targets of '" << name << "' should be assigned as string!" << endl;
                        return true;
                    }
                    if (tt == name) {
                        cerr << "build::error: self additional target detected at '" << name << "'!" << endl;
                        return true;
                    }
                    if (execute(js, tt)) return true;
                }
            }

            if (!force) {
                bool exec = false;
                if (!t["onchange"].empty()) {
                    for (auto& c : t["onchange"]) {
                        string cf = parse(js, (string)c);
                        if (changed(cf)) exec = true;
                    }

                    if (!exec) return false;
                }
            }

            string cmd;
            for (auto& a : t["commands"]) {
                array<char, 128> buffer;

                // apply 'directory' parameter 
                auto path = fs::current_path();
                if (t["directory"].is_string()) {
                    auto dir = parse(js, t["directory"]);
                    try {
                        fs::current_path(dir);
                    } catch(fs::filesystem_error& er) {
                        cerr << "build::error: No such file or directory '" << dir << "' while execute '" <<  name << "' target." << endl;
                        exit(1);
                    }
                }

                string cmd = parse(js, (string)a);
                cout << cmd << endl;
                cmd.append(" 2>&1");
                auto pipe = popen(cmd.c_str(), "r");
                if (!pipe) throw runtime_error("build:error: execute failed: " + cmd);

                // output cout in popen
                while (fgets(buffer.data(), 128, pipe) != NULL) {
                    cout << buffer.data();
                }

                // auto rc = 
                pclose(pipe);
                // if (rc != EXIT_SUCCESS) {
                //     return true;
                // }
                fs::current_path(path);
            }
            height--;
            if (height == 0) force = false;
            return false;
        }
    }
    

    cerr << "build::error: target '" << name << "' doesn't exist!" << endl;
    return true;
}

bool is_invalid(json& js) {
    bool invalid = false;
    auto targets = js["targets"];
    for (auto& t : targets) {
        if (!t.is_object()) {
            cerr << "build::error: Parse error: data inside 'targets' should be assigned as object." << endl;
            invalid = true;
        } else {
            if (t["name"].is_null()) {
                cerr << "build::error: Parse error: found target with no name." << endl;
                invalid = true;
            } else {
                if (!t["name"].is_string()) {
                    cerr << "build::error: Parse error: 'name' should be assigned as string." << endl;
                    invalid = true;
                }
            }
            if (t["commands"].is_null()) {
                cerr << "build::error: Parse error: found target with no commands." << endl;
                invalid = true;
            } else {
                if (!t["commands"].is_array()) {
                    cerr << "build::error: Parse error: 'commands' should be assigned as array." << endl;
                    invalid = true;
                } else {
                    for (auto& a : t["commands"]) {
                        if (!a.is_string()) {
                            cerr << "build::error: Parse error: data inside 'commands' should be assigned as string." << endl;
                            invalid = true;
                        }
                    }
                }
            }
        }
    }

    return invalid;
}

int main(int argc, char* argv[]) {
    json js;
    string f = get_file_data("build.json");
    if (f.length() == 0) {
        return 1;
    }
    
    try {
        js = json::parse(f, nullptr, true, true);
    } catch(json::parse_error& ex) {
        cerr << "build::error: Parse error: " << ex.what() << endl;
        return 1;
    }

    auto targets = js["targets"];
    if (targets.is_null() || targets.empty()) {
        cerr << "build::error: No targets or empty targets." << endl;
        return 1;
    }

    if (!targets.is_array()) {
        cerr << "build::error: Parse error: 'targets' should be assigned as array." << endl;
        return 1;
    }

    if (is_invalid(js)) return 1;

    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (string(argv[i]) == "-help") {
                show::info();
                continue;
            } else if (string(argv[i]) == "-targets") {
                show::targets(js);
                continue;
            } else if (string(argv[i]) == "-force") {
                force = true;
                continue;
            }

            if (execute(js, argv[i])) {
                cerr << "build::error: Target error: target '" << argv[i] << "' got error!" << endl;
                return 1;
            }
        }
    } else {
        if (execute(js)) {
            cerr << "build::error: Target error: target '" << "all" << "' got error!" << endl;
            return 1;
        }
    }
    
    return 0;
}