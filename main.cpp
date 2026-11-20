
// University Student Record System - AVL Tree vs B+ Tree (simple single-file version)
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <iomanip>
using namespace std;

// ---------------------------------------------------------------- Student
struct Student {
    int id = 0;
    string name, dept;
    int semester = 0;
    float cgpa = 0;
    string city;
};

void printStudent(const Student& s) {
    cout << left << setw(8) << s.id << setw(18) << s.name << setw(7) << s.dept
         << setw(5) << s.semester << setw(6) << fixed << setprecision(1) << s.cgpa
         << s.city << "\n";
}

// ---------------------------------------------------------------- AVL TREE
struct AVLNode {
    Student data;
    AVLNode *left = nullptr, *right = nullptr;
    int height = 1;
};

int hgt(AVLNode* n) { return n ? n->height : 0; }
void upd(AVLNode* n) { n->height = 1 + max(hgt(n->left), hgt(n->right)); }

AVLNode* rotRight(AVLNode* y) {          // fixes left-heavy
    AVLNode* x = y->left;
    y->left = x->right;
    x->right = y;
    upd(y); upd(x);
    return x;
}
AVLNode* rotLeft(AVLNode* x) {           // fixes right-heavy
    AVLNode* y = x->right;
    x->right = y->left;
    y->left = x;
    upd(x); upd(y);
    return y;
}

// Recalculate height, rotate if balance factor is outside -1..1
AVLNode* balanceNode(AVLNode* n) {
    upd(n);
    int bf = hgt(n->left) - hgt(n->right);
    if (bf > 1) {
        if (hgt(n->left->left) < hgt(n->left->right)) n->left = rotLeft(n->left);   // LR case
        return rotRight(n);                                                          // LL case
    }
    if (bf < -1) {
        if (hgt(n->right->right) < hgt(n->right->left)) n->right = rotRight(n->right); // RL case
        return rotLeft(n);                                                             // RR case
    }
    return n;
}

class AVLTree {
    AVLNode* root = nullptr;
    int count = 0;

    AVLNode* insert(AVLNode* n, const Student& s, bool& ok) {
        if (!n) { AVLNode* x = new AVLNode; x->data = s; return x; }
        if (s.id < n->data.id) n->left = insert(n->left, s, ok);
        else if (s.id > n->data.id) n->right = insert(n->right, s, ok);
        else { ok = false; return n; }          // duplicate
        return balanceNode(n);
    }
    AVLNode* remove(AVLNode* n, int id, bool& ok) {
        if (!n) return nullptr;
        if (id < n->data.id) n->left = remove(n->left, id, ok);
        else if (id > n->data.id) n->right = remove(n->right, id, ok);
        else {
            ok = true;
            if (!n->left || !n->right) {        // 0 or 1 child
                AVLNode* child = n->left ? n->left : n->right;
                delete n;
                return child;
            }
            AVLNode* s = n->right;              // 2 children: use smallest on the right
            while (s->left) s = s->left;
            n->data = s->data;
            bool tmp;
            n->right = remove(n->right, s->data.id, tmp);
        }
        return balanceNode(n);
    }
    void range(AVLNode* n, int lo, int hi, vector<Student>& out) {
        if (!n) return;
        if (lo < n->data.id) range(n->left, lo, hi, out);
        if (lo <= n->data.id && n->data.id <= hi) out.push_back(n->data);
        if (hi > n->data.id) range(n->right, lo, hi, out);
    }
    void inorder(AVLNode* n, vector<Student>& out) {
        if (!n) return;
        inorder(n->left, out);
        out.push_back(n->data);
        inorder(n->right, out);
    }
    void destroy(AVLNode* n) { if (n) { destroy(n->left); destroy(n->right); delete n; } }

public:
    ~AVLTree() { destroy(root); }
    bool insert(const Student& s) { bool ok = true; root = insert(root, s, ok); if (ok) count++; return ok; }
    bool remove(int id) { bool ok = false; root = remove(root, id, ok); if (ok) count--; return ok; }
    const Student* search(int id) {
        AVLNode* n = root;
        while (n) {
            if (id == n->data.id) return &n->data;
            n = (id < n->data.id) ? n->left : n->right;
        }
        return nullptr;
    }
    vector<Student> rangeSearch(int lo, int hi) { vector<Student> v; range(root, lo, hi, v); return v; }
    vector<Student> inorder() { vector<Student> v; inorder(root, v); return v; }
    int size() const { return count; }
    int height() const { return hgt(root); }
};

// ---------------------------------------------------------------- B+ TREE
struct BNode {
    bool leaf;
    vector<int> keys;
    vector<Student> recs;      // only used in leaves
    vector<BNode*> kids;       // only used in internal nodes
    BNode* next = nullptr;     // leaf -> next leaf
    BNode(bool l) : leaf(l) {}
};

class BPlusTree {
    BNode* root;
    int maxKeys;               // max keys per node (order - 1)
    int count = 0;

    // Insert into subtree n. If n splits, newNode = right half and upKey = key to push up.
    void insertRec(BNode* n, const Student& s, bool& dup, int& upKey, BNode*& newNode) {
        newNode = nullptr;
        if (n->leaf) {
            int i = lower_bound(n->keys.begin(), n->keys.end(), s.id) - n->keys.begin();
            if (i < (int)n->keys.size() && n->keys[i] == s.id) { dup = true; return; }
            n->keys.insert(n->keys.begin() + i, s.id);
            n->recs.insert(n->recs.begin() + i, s);
            if ((int)n->keys.size() > maxKeys) {            // leaf split
                int mid = n->keys.size() / 2;
                BNode* r = new BNode(true);
                r->keys.assign(n->keys.begin() + mid, n->keys.end());
                r->recs.assign(n->recs.begin() + mid, n->recs.end());
                n->keys.erase(n->keys.begin() + mid, n->keys.end());
                n->recs.erase(n->recs.begin() + mid, n->recs.end());
                r->next = n->next;
                n->next = r;
                upKey = r->keys[0];                         // key is COPIED up
                newNode = r;
            }
            return;
        }
        int i = upper_bound(n->keys.begin(), n->keys.end(), s.id) - n->keys.begin();
        int k; BNode* nn;
        insertRec(n->kids[i], s, dup, k, nn);
        if (!nn) return;
        n->keys.insert(n->keys.begin() + i, k);
        n->kids.insert(n->kids.begin() + i + 1, nn);
        if ((int)n->keys.size() > maxKeys) {                // internal split
            int mid = n->keys.size() / 2;
            BNode* r = new BNode(false);
            upKey = n->keys[mid];                           // key is MOVED up
            r->keys.assign(n->keys.begin() + mid + 1, n->keys.end());
            r->kids.assign(n->kids.begin() + mid + 1, n->kids.end());
            n->keys.erase(n->keys.begin() + mid, n->keys.end());
            n->kids.erase(n->kids.begin() + mid + 1, n->kids.end());
            newNode = r;
        }
    }
    BNode* findLeaf(int id) {
        BNode* n = root;
        while (!n->leaf) n = n->kids[upper_bound(n->keys.begin(), n->keys.end(), id) - n->keys.begin()];
        return n;
    }
    void destroy(BNode* n) {
        if (!n->leaf) for (BNode* c : n->kids) destroy(c);
        delete n;
    }

public:
    BPlusTree(int maxKeysPerNode = 3) : root(new BNode(true)), maxKeys(maxKeysPerNode) {}
    ~BPlusTree() { destroy(root); }

    bool insert(const Student& s) {
        bool dup = false; int k; BNode* nn;
        insertRec(root, s, dup, k, nn);
        if (dup) return false;
        if (nn) {                                           // root split -> tree grows
            BNode* r = new BNode(false);
            r->keys.push_back(k);
            r->kids.push_back(root);
            r->kids.push_back(nn);
            root = r;
        }
        count++;
        return true;
    }
    const Student* search(int id) {
        BNode* leaf = findLeaf(id);
        int i = lower_bound(leaf->keys.begin(), leaf->keys.end(), id) - leaf->keys.begin();
        if (i < (int)leaf->keys.size() && leaf->keys[i] == id) return &leaf->recs[i];
        return nullptr;
    }
    // SIMPLE delete: removes from the leaf only (no borrow/merge). See notes.
    bool remove(int id) {
        BNode* leaf = findLeaf(id);
        int i = lower_bound(leaf->keys.begin(), leaf->keys.end(), id) - leaf->keys.begin();
        if (i >= (int)leaf->keys.size() || leaf->keys[i] != id) return false;
        leaf->keys.erase(leaf->keys.begin() + i);
        leaf->recs.erase(leaf->recs.begin() + i);
        count--;
        return true;
    }
    vector<Student> rangeSearch(int lo, int hi) {           // find first leaf, follow next pointers
        vector<Student> out;
        for (BNode* leaf = findLeaf(lo); leaf; leaf = leaf->next)
            for (size_t i = 0; i < leaf->keys.size(); i++) {
                if (leaf->keys[i] > hi) return out;
                if (leaf->keys[i] >= lo) out.push_back(leaf->recs[i]);
            }
        return out;
    }
    vector<Student> inorder() {
        vector<Student> out;
        BNode* n = root;
        while (!n->leaf) n = n->kids[0];
        for (; n; n = n->next) for (auto& s : n->recs) out.push_back(s);
        return out;
    }
    int size() const { return count; }
    int height() const { int h = 1; for (BNode* n = root; !n->leaf; n = n->kids[0]) h++; return h; }
};

// ---------------------------------------------------------------- CSV save/load (Challenge 3)
const string FILE_NAME = "students.csv";

void saveCSV(AVLTree& t) {
    ofstream f(FILE_NAME);
    f << "id,name,dept,semester,cgpa,city\n";
    for (auto& s : t.inorder())
        f << s.id << "," << s.name << "," << s.dept << "," << s.semester << "," << s.cgpa << "," << s.city << "\n";
}

vector<Student> loadCSV() {
    vector<Student> v;
    ifstream f(FILE_NAME);
    string line;
    getline(f, line);                                       // skip header
    while (getline(f, line)) {
        stringstream ss(line);
        string id, name, dept, sem, cgpa, city;
        getline(ss, id, ','); getline(ss, name, ','); getline(ss, dept, ',');
        getline(ss, sem, ','); getline(ss, cgpa, ','); getline(ss, city, ',');
        try { v.push_back({stoi(id), name, dept, stoi(sem), stof(cgpa), city}); }
        catch (...) { cout << "Skipped bad line: " << line << "\n"; }
    }
    return v;
}

// ---------------------------------------------------------------- Input helpers
int readInt(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) exit(0);
        try { size_t p; int v = stoi(line, &p); if (p == line.size()) return v; } catch (...) {}
        cout << "Invalid number, try again.\n";
    }
}
float readFloat(const string& prompt) {
    while (true) {
        cout << prompt;
        string line;
        if (!getline(cin, line)) exit(0);
        try { size_t p; float v = stof(line, &p); if (p == line.size()) return v; } catch (...) {}
        cout << "Invalid number, try again.\n";
    }
}
string readText(const string& prompt) {
    string s;
    do { cout << prompt; if (!getline(cin, s)) exit(0); } while (s.empty());
    return s;
}

// ---------------------------------------------------------------- Benchmark (Challenge 8)
double msSince(chrono::steady_clock::time_point t) {
    return chrono::duration<double, milli>(chrono::steady_clock::now() - t).count();
}

void runBenchmark() {
    vector<int> sizes = {1000, 10000, 100000};
    ofstream csv("results.csv");
    csv << "n,operation,avl_ms,bplus_ms\n";
    cout << "\n(B+ Tree uses 32 keys per node here; same random data for both trees)\n";
    cout << left << setw(9) << "N" << setw(16) << "Operation" << setw(12) << "AVL (ms)"
         << setw(12) << "B+ (ms)" << "Winner\n" << string(58, '-') << "\n";

    for (int n : sizes) {
        mt19937 rng(42);                                    // fixed seed = same data every run
        vector<int> ids(n);
        for (int i = 0; i < n; i++) ids[i] = i + 1;
        shuffle(ids.begin(), ids.end(), rng);

        AVLTree avl; BPlusTree bpt(32);
        double a[4], b[4];
        long long found = 0;          // using the results stops the compiler skipping the searches
        auto t = chrono::steady_clock::now();

        t = chrono::steady_clock::now();
        for (int id : ids) avl.insert({id, "Name", "CSE", 3, 8.0f, "City"});
        a[0] = msSince(t);
        t = chrono::steady_clock::now();
        for (int id : ids) bpt.insert({id, "Name", "CSE", 3, 8.0f, "City"});
        b[0] = msSince(t);

        t = chrono::steady_clock::now();
        for (int id : ids) if (avl.search(id)) found++;
        a[1] = msSince(t);
        t = chrono::steady_clock::now();
        for (int id : ids) if (bpt.search(id)) found++;
        b[1] = msSince(t);

        int queries = 1000, span = max(1, n / 10), sink = 0;        // range = 10% of key space
        t = chrono::steady_clock::now();
        for (int q = 0; q < queries; q++) { int lo = 1 + rng() % (n - span + 1); sink += avl.rangeSearch(lo, lo + span).size(); }
        a[2] = msSince(t);
        rng.seed(7);
        t = chrono::steady_clock::now();
        for (int q = 0; q < queries; q++) { int lo = 1 + rng() % (n - span + 1); sink += bpt.rangeSearch(lo, lo + span).size(); }
        b[2] = msSince(t);
        if (found < 0 || sink < 0) cout << "";   // keep results 'used'

        t = chrono::steady_clock::now();
        for (int i = 0; i < n / 5; i++) avl.remove(ids[i]);         // delete 20%
        a[3] = msSince(t);
        t = chrono::steady_clock::now();
        for (int i = 0; i < n / 5; i++) bpt.remove(ids[i]);
        b[3] = msSince(t);

        const char* ops[4] = {"Insert", "Search", "Range (1000x)", "Delete 20%"};
        for (int i = 0; i < 4; i++) {
            cout << left << setw(9) << n << setw(16) << ops[i] << setw(12) << fixed << setprecision(3)
                 << a[i] << setw(12) << b[i] << (a[i] < b[i] ? "AVL" : "B+ Tree") << "\n";
            csv << n << "," << ops[i] << "," << a[i] << "," << b[i] << "\n";
        }
        cout << "  heights at N=" << n << " (before delete): AVL=" << avl.height()
             << ", B+=" << bpt.height() << "\n" << string(58, '-') << "\n";
    }
    cout << "Saved results.csv\n";
}

// ---------------------------------------------------------------- Main menu
AVLTree avl;
BPlusTree bpt;   // default: max 3 keys per node
bool useAVL = true;

void showList(const vector<Student>& v) {
    cout << left << setw(8) << "ID" << setw(18) << "Name" << setw(7) << "Dept"
         << setw(5) << "Sem" << setw(6) << "CGPA" << "City\n" << string(55, '-') << "\n";
    for (auto& s : v) printStudent(s);
    cout << v.size() << " record(s)\n";
}

bool addStudent(const Student& s, bool quiet = false) {
    if (avl.search(s.id)) {
        if (!quiet) cout << "ERROR: Student ID " << s.id << " already exists.\n";
        return false;
    }
    avl.insert(s); bpt.insert(s);                           // keep both trees identical
    return true;
}

int main() {
    // Load saved data (or create it from the sample dataset)
    vector<Student> loaded = loadCSV();
    if (loaded.empty()) {
        loaded = {
            {1001,"Aditi Sharma","CSE",5,8.7f,"Mumbai"},   {1002,"Rahul Patil","IT",5,7.9f,"Pune"},
            {1003,"Neha Desai","AIML",3,9.1f,"Nashik"},     {1004,"Arjun Mehta","CSE",7,8.2f,"Mumbai"},
            {1005,"Sneha Kulkarni","IT",3,8.5f,"Pune"},     {1006,"Rohan Shah","AIML",5,7.6f,"Thane"},
            {1007,"Priya Nair","CSE",3,9.3f,"Mumbai"},      {1008,"Karan Joshi","IT",7,8.0f,"Nagpur"},
            {1009,"Isha Gupta","AIML",5,8.9f,"Pune"},       {1010,"Aditya Rao","CSE",7,7.8f,"Mumbai"}};
    }
    for (auto& s : loaded) addStudent(s, true);
    saveCSV(avl);
    cout << "Loaded " << avl.size() << " students.\n";

    while (true) {
        cout << "\n===== Student Records (using " << (useAVL ? "AVL Tree" : "B+ Tree") << ") =====\n"
             << "1. Add student        2. Delete student\n"
             << "3. Search by ID       4. Range search\n"
             << "5. Display sorted     6. Switch tree (AVL / B+)\n"
             << "7. Run benchmark      8. Students with CGPA > 9.0\n"
             << "0. Exit\n";
        int c = readInt("Choice: ");

        if (c == 1) {
            Student s;
            s.id = readInt("ID: ");
            if (s.id <= 0) { cout << "ID must be positive.\n"; continue; }
            if (avl.search(s.id)) { cout << "ERROR: Student ID " << s.id << " already exists.\n"; continue; }
            s.name = readText("Name: ");
            s.dept = readText("Department: ");
            s.semester = readInt("Semester (1-8): ");
            s.cgpa = readFloat("CGPA (0-10): ");
            s.city = readText("City: ");
            if (s.semester < 1 || s.semester > 8 || s.cgpa < 0 || s.cgpa > 10) { cout << "Invalid semester or CGPA.\n"; continue; }
            addStudent(s);
            saveCSV(avl);
            cout << "Student added.\n";
        } else if (c == 2) {
            int id = readInt("ID to delete: ");
            bool ok1 = avl.remove(id), ok2 = bpt.remove(id);
            cout << (ok1 && ok2 ? "Deleted.\n" : "Student not found.\n");
            saveCSV(avl);
        } else if (c == 3) {
            int id = readInt("ID to search: ");
            const Student* s = useAVL ? avl.search(id) : bpt.search(id);
            if (s) printStudent(*s); else cout << "Student not found.\n";
        } else if (c == 4) {
            int lo = readInt("From ID: "), hi = readInt("To ID: ");
            showList(useAVL ? avl.rangeSearch(lo, hi) : bpt.rangeSearch(lo, hi));
        } else if (c == 5) {
            showList(useAVL ? avl.inorder() : bpt.inorder());
        } else if (c == 6) {
            useAVL = !useAVL;
        } else if (c == 7) {
            runBenchmark();
        } else if (c == 8) {
            vector<Student> top;
            for (auto& s : (useAVL ? avl.inorder() : bpt.inorder())) if (s.cgpa > 9.0f) top.push_back(s);
            showList(top);
        } else if (c == 0) {
            saveCSV(avl);
            cout << "Saved. Goodbye!\n";
            break;
        } else cout << "Invalid choice.\n";
    }
}
