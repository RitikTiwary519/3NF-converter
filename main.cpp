#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include <string>

using namespace std;

struct FunctionalDependency {
    set<string> lhs;
    set<string> rhs;
};

// Compute attribute closure
set<string> closure(set<string> attrs, const vector<FunctionalDependency>& fds) {
    set<string> result = attrs;
    bool changed;
    do {
        changed = false;
        for (const auto& fd : fds) {
            if (includes(result.begin(), result.end(), fd.lhs.begin(), fd.lhs.end())) {
                size_t oldSize = result.size();
                result.insert(fd.rhs.begin(), fd.rhs.end());
                if (result.size() > oldSize)
                    changed = true;
            }
        }
    } while (changed);
    return result;
}

// Parse DDL input
void parseDDL(string ddl, set<string>& attributes, set<string>& primaryKey) {
    ddl.erase(remove(ddl.begin(), ddl.end(), '\n'), ddl.end());

    // Convert to uppercase (optional for case-insensitive parsing)
    string upperDDL = ddl;
    transform(upperDDL.begin(), upperDDL.end(), upperDDL.begin(), ::toupper);

    size_t pkStart = upperDDL.find("PRIMARY KEY");
    if (pkStart != string::npos) {
        size_t open = ddl.find("(", pkStart);
        size_t close = ddl.find(")", open);
        string pkList = ddl.substr(open + 1, close - open - 1);
        stringstream ss(pkList);
        string attr;
        while (getline(ss, attr, ',')) {
            primaryKey.insert(attr);
        }
    }

    // Find all lines with attributes
    size_t start = ddl.find("(") + 1;
    size_t end = ddl.rfind(")");
    string inner = ddl.substr(start, end - start);

    stringstream ss(inner);
    string line;
    while (getline(ss, line, ',')) {
        stringstream ls(line);
        string attr;
        ls >> attr;
        if (attr != "PRIMARY" && attr != "")
            attributes.insert(attr);
    }
}

// Parse FD input like A->B,C
vector<FunctionalDependency> parseFDs() {
    vector<FunctionalDependency> fds;
    cout << "Enter Functional Dependencies (e.g., A->B,C), type END to stop:\n";
    string line;
    while (getline(cin, line)) {
        if (line == "END") break;
        size_t pos = line.find("->");
        if (pos == string::npos) continue;

        FunctionalDependency fd;
        string lhsStr = line.substr(0, pos);
        string rhsStr = line.substr(pos + 2);

        stringstream ss(lhsStr);
        string token;
        while (getline(ss, token, ',')) fd.lhs.insert(token);

        stringstream rs(rhsStr);
        while (getline(rs, token, ',')) fd.rhs.insert(token);

        fds.push_back(fd);
    }
    return fds;
}

// Find candidate keys
vector<set<string>> findCandidateKeys(const set<string>& allAttrs, const vector<FunctionalDependency>& fds) {
    vector<set<string>> keys;
    vector<string> attrs(allAttrs.begin(), allAttrs.end());
    int n = attrs.size();

    for (int i = 1; i < (1 << n); ++i) {
        set<string> subset;
        for (int j = 0; j < n; ++j)
            if (i & (1 << j)) subset.insert(attrs[j]);

        set<string> closureSet = closure(subset, fds);
        if (closureSet == allAttrs) {
            bool isMinimal = true;
            for (const auto& key : keys) {
                if (includes(subset.begin(), subset.end(), key.begin(), key.end())) {
                    isMinimal = false;
                    break;
                }
            }
            if (isMinimal) keys.push_back(subset);
        }
    }

    return keys;
}

// Normalize to 3NF
void normalize3NF(const vector<FunctionalDependency>& fds, const vector<set<string>>& candidateKeys) {
    cout << "\n=== 3NF Decomposition ===\n";
    int relNum = 1;
    set<set<string>> relations;

    for (const auto& fd : fds) {
        set<string> rel = fd.lhs;
        rel.insert(fd.rhs.begin(), fd.rhs.end());
        relations.insert(rel);
    }

    bool keyCovered = false;
    for (const auto& key : candidateKeys) {
        if (relations.count(key)) {
            keyCovered = true;
            break;
        }
    }

    if (!keyCovered) relations.insert(candidateKeys[0]);

    for (const auto& rel : relations) {
        cout << "CREATE TABLE R" << relNum++ << " (\n";
        for (const auto& attr : rel)
            cout << "    " << attr << " VARCHAR(255),\n";
        cout << "    PRIMARY KEY (";
        for (auto it = rel.begin(); it != rel.end(); ++it) {
            cout << *it;
            if (next(it) != rel.end()) cout << ", ";
        }
        cout << ")\n);\n";
    }
}

int main() {
    set<string> attributes, primaryKey;
    string line, ddl;
    cout << "Enter SQL DDL (Type 'END' on a new line to finish):\n";
    while (getline(cin, line)) {
        if (line == "END") break;
        ddl += line + "\n";
    }

    parseDDL(ddl, attributes, primaryKey);

    cout << "\nParsed Attributes:\n";
    for (const string& attr : attributes) cout << attr << " ";
    cout << "\nPrimary Key: ";
    for (const string& pk : primaryKey) cout << pk << " ";
    cout << "\n";

    auto fds = parseFDs();
    auto keys = findCandidateKeys(attributes, fds);

    cout << "\nCandidate Keys:\n";
    for (const auto& key : keys) {
        for (const auto& attr : key) cout << attr << " ";
        cout << "\n";
    }

    normalize3NF(fds, keys);
    return 0;
}
