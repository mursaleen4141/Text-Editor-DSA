#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <limits>
using namespace std;
// Dynamic array used to store the lines of the document.
class LineArray {
private:
    string* data;
    int capacity;
    int count;
    void resizeArray() {
        int newCapacity = capacity * 2;
        string* newData = new string[newCapacity];
        for (int i = 0; i < count; i++)
            newData[i] = data[i];
        delete[] data;
        data = newData;
        capacity = newCapacity;
    }
public:
    LineArray() : capacity(4), count(0) {
        data = new string[capacity];
    }
    LineArray(const LineArray& other) : capacity(other.capacity), count(other.count) {
        data = new string[capacity];
        for (int i = 0; i < count; i++)
            data[i] = other.data[i];
    }
    LineArray& operator=(const LineArray& other) {
        if (this != &other) {
            delete[] data;
            capacity = other.capacity;
            count = other.count;
            data = new string[capacity];
            for (int i = 0; i < count; i++)
                data[i] = other.data[i];
        }
        return *this;
    }
    ~LineArray() { delete[] data; }
    int size() const { return count; }
    string& operator[](int index) { return data[index]; }
    const string& operator[](int index) const { return data[index]; }
    void add(const string& value) {
        if (count == capacity) resizeArray();
        data[count++] = value;
    }
    void insert(int position, const string& value) {
        if (position < 0 || position > count) return;
        if (count == capacity) resizeArray();
        for (int i = count; i > position; i--)
            data[i] = data[i - 1];
        data[position] = value;
        count++;
    }
    void remove(int position) {
        if (position < 0 || position >= count) return;
        for (int i = position; i < count - 1; i++)
            data[i] = data[i + 1];
        count--;
    }
    void clear() { count = 0; }
};
// Linked-list stack. Each node stores one previous document version.
class UndoStack {
private:
    struct Node {
        string document;
        Node* next;
        Node(const string& text) : document(text), next(NULL) {}
    };
    Node* topNode;
    int count;
    static const int MAX_UNDO = 50;
public:
    UndoStack() : topNode(NULL), count(0) {}
    ~UndoStack() { clear(); }
    void push(const string& text) {
        Node* node = new Node(text);
        node->next = topNode;
        topNode = node;
        count++;
        // Keep only the 50 most recent states.
        if (count > MAX_UNDO) {
            Node* current = topNode;
            for (int i = 1; i < MAX_UNDO; i++)
                current = current->next;
            Node* extra = current->next;
            current->next = NULL;
            while (extra != NULL) {
                Node* next = extra->next;
                delete extra;
                extra = next;
            }
            count = MAX_UNDO;
        }
    }
    bool pop(string& text) {
        if (topNode == NULL) return false;
        Node* oldTop = topNode;
        text = oldTop->document;
        topNode = topNode->next;
        delete oldTop;
        count--;
        return true;
    }
    void clear() {
        while (topNode != NULL) {
            Node* oldTop = topNode;
            topNode = topNode->next;
            delete oldTop;
        }
        count = 0;
    }
};
class TextEditor {
private:
    LineArray lines;
    UndoStack undoHistory;
    string currentFile;
    bool changed;
    int readNumber(const string& prompt) {
        string input;
        int number;
        while (true) {
            cout << prompt;
            if (!getline(cin, input)) return -1;
            stringstream ss(input);
            char extra;
            if (ss >> number && !(ss >> extra)) return number;
            cout << "Please enter a valid whole number.\n";
        }
    }
    string documentText() const {
        string text;
        for (int i = 0; i < lines.size(); i++) {
            text += lines[i];
            if (i < lines.size() - 1) text += '\n';
        }
        return text;
    }
    void loadText(const string& text) {
        lines.clear();
        if (text.empty()) {
            lines.add("");
            return;
        }
        string line;
        for (size_t i = 0; i < text.length(); i++) {
            if (text[i] == '\n') {
                if (!line.empty() && line[line.length() - 1] == '\r')
                    line.erase(line.length() - 1);
                lines.add(line);
                line.clear();
            } else {
                line += text[i];
            }
        }
        lines.add(line);
    }
    void saveUndoState() { undoHistory.push(documentText()); }
    bool validLine(int lineNumber) const {
        return lineNumber >= 1 && lineNumber <= lines.size();
    }
    bool occursAt(int lineIndex, int columnIndex, const string& target) const {
        if (target.empty() || columnIndex < 0) return false;
        if (columnIndex + static_cast<int>(target.length()) >
            static_cast<int>(lines[lineIndex].length())) return false;
        for (int i = 0; i < static_cast<int>(target.length()); i++)
            if (lines[lineIndex][columnIndex + i] != target[i]) return false;
        return true;
    }
    void findText(const string& target, bool printLocations, int& total) const {
        total = 0;
        if (target.empty()) return;
        for (int i = 0; i < lines.size(); i++) {
            for (int j = 0; j + static_cast<int>(target.length()) <=
                            static_cast<int>(lines[i].length()); j++) {
                if (occursAt(i, j, target)) {
                    total++;
                    if (printLocations)
                        cout << "Line " << i + 1 << ", column " << j + 1 << '\n';
                }
            }
        }
    }
    bool saveTo(const string& name) {
        ofstream file(name.c_str());
        if (!file) {
            cout << "Error: Could not create or write to that file.\n";
            return false;
        }
        file << documentText();
        file.close();
        currentFile = name;
        changed = false;
        cout << "Document saved successfully.\n";
        return true;
    }
public:
    TextEditor() : currentFile(""), changed(false) { lines.add(""); }
    void newDocument() {
        lines.clear();
        lines.add("");
        undoHistory.clear();
        currentFile = "";
        changed = false;
        cout << "New document created.\n";
    }
    void openFile() {
        cout << "Enter file name: ";
        string name;
        getline(cin, name);
        if (name.empty()) {
            cout << "File name cannot be empty.\n";
            return;
        }
        ifstream file(name.c_str(), ios::binary);
        if (!file) {
            cout << "Error: File could not be opened. Check its name and location.\n";
            return;
        }
        string text;
        char ch;
        while (file.get(ch)) text += ch;
        file.close();
        loadText(text);
        currentFile = name;
        undoHistory.clear();
        changed = false;
        cout << "File opened successfully.\n";
    }
    void saveFile() {
        if (currentFile.empty()) {
            saveAs();
        } else {
            saveTo(currentFile);
        }
    }
    void saveAs() {
        cout << "Enter file name: ";
        string name;
        getline(cin, name);
        if (name.empty()) {
            cout << "File name cannot be empty.\n";
            return;
        }
        saveTo(name);
    }
    void display() const {
        cout << "\n----------- DOCUMENT -----------\n";
        for (int i = 0; i < lines.size(); i++)
            cout << i + 1 << ": " << lines[i] << '\n';
        cout << "--------------------------------\n";
    }
    void showInformation() const {
        int characters = 0;
        for (int i = 0; i < lines.size(); i++)
            characters += static_cast<int>(lines[i].length());
        cout << "Lines: " << lines.size() << '\n';
        cout << "Characters (not counting line breaks): " << characters << '\n';
        cout << "File: " << (currentFile.empty() ? "Not saved" : currentFile) << '\n';
        cout << "Status: " << (changed ? "Modified" : "Saved") << '\n';
    }
    void insertText() {
        int line = readNumber("Line number: ");
        int column = readNumber("Column (1 means beginning): ");
        if (!validLine(line) || column < 1 ||
            column > static_cast<int>(lines[line - 1].length()) + 1) {
            cout << "Invalid line or column.\n";
            return;
        }
        cout << "Text to insert: ";
        string text;
        getline(cin, text);
        if (text.empty()) {
            cout << "Nothing to insert.\n";
            return;
        }
        saveUndoState();
        lines[line - 1].insert(column - 1, text);
        changed = true;
        cout << "Text inserted.\n";
    }
    void deleteCharacter() {
        int line = readNumber("Line number: ");
        int column = readNumber("Column of character to delete: ");
        if (!validLine(line) || column < 1 ||
            column > static_cast<int>(lines[line - 1].length())) {
            cout << "Invalid line or column.\n";
            return;
        }
        saveUndoState();
        lines[line - 1].erase(column - 1, 1);
        changed = true;
        cout << "Character deleted.\n";
    }
    void deleteWord() {
        int line = readNumber("Line number: ");
        int column = readNumber("Column where the word starts: ");
        if (!validLine(line) || column < 1 ||
            column > static_cast<int>(lines[line - 1].length())) {
            cout << "Invalid line or column.\n";
            return;
        }
        string& text = lines[line - 1];
        int start = column - 1;
        if (text[start] == ' ' || text[start] == '\t') {
            cout << "Choose a column containing a word character.\n";
            return;
        }
        int end = start;
        while (end < static_cast<int>(text.length()) &&
               text[end] != ' ' && text[end] != '\t') end++;
        saveUndoState();
        text.erase(start, end - start);
        changed = true;
        cout << "Word deleted.\n";}
    void deleteRange() {
        int line = readNumber("Line number: ");
        int first = readNumber("Starting column: ");
        int last = readNumber("Ending column (inclusive): ");
        if (!validLine(line) || first < 1 || last < first ||
            last > static_cast<int>(lines[line - 1].length())) {
            cout << "Invalid text range.\n";
            return;}
        saveUndoState();
        lines[line - 1].erase(first - 1, last - first + 1);
        changed = true;
        cout << "Text range deleted.\n";
    }
    void insertNewLine() {
        int line = readNumber("Insert new line after line number: ");
        if (!validLine(line)) {
            cout << "Invalid line number.\n";
            return;
        }
        saveUndoState();
        lines.insert(line, "");
        changed = true;
        cout << "New line inserted successfully.\n";}

    void deleteLines() {
        int first = readNumber("First line to delete: ");
        int last = readNumber("Last line to delete: ");
        if (first < 1 || last < first || last > lines.size()) {
            cout << "Invalid line range.\n";
            return;
        }
        saveUndoState();
        for (int i = last - 1; i >= first - 1; i--)
            lines.remove(i);
        if (lines.size() == 0) lines.add("");
        changed = true;
        cout << "Selected line(s) deleted.\n";
    }
    void search() const {
        cout << "Text to search: ";
        string target;
        getline(cin, target);
        if (target.empty()) {
            cout << "Search text cannot be empty.\n";
            return;
        }
        int total;
        findText(target, true, total);
        if (total == 0) cout << "No match found.\n";
        else cout << "Total matches: " << total << '\n';
    }
    void replaceOne() {
        cout << "Text to find: ";
        string target;
        getline(cin, target);
        if (target.empty()) {
            cout << "Search text cannot be empty.\n";
            return;
        }
        int total;
        findText(target, true, total);
        if (total == 0) {
            cout << "No match found.\n";
            return;
        }
        int line = readNumber("Line number of occurrence to replace: ");
        int column = readNumber("Starting column of occurrence: ");
        if (!validLine(line) || !occursAt(line - 1, column - 1, target)) {
            cout << "That location does not contain the search text.\n";
            return;
        }
        cout << "Replacement text: ";
        string replacement;
        getline(cin, replacement);
        saveUndoState();
        lines[line - 1].replace(column - 1, target.length(), replacement);
        changed = true;
        cout << "One occurrence replaced.\n";
    }
    void replaceAll() {
        cout << "Text to find: ";
        string target;
        getline(cin, target);
        if (target.empty()) {
            cout << "Search text cannot be empty.\n";
            return;}
        cout << "Replacement text: ";
        string replacement;
        getline(cin, replacement);
        int total;
        findText(target, false, total);
        if (total == 0) {
            cout << "No match found.\n";
            return;}
        saveUndoState();
        int replacements = 0;
        for (int i = 0; i < lines.size(); i++) {
            size_t position = 0;
            while (position + target.length() <= lines[i].length()) {
                if (occursAt(i, static_cast<int>(position), target)) {
                    lines[i].replace(position, target.length(), replacement);
                    position += replacement.length();
                    replacements++;
                } else {
                    position++;}
            }
        }
        changed = true;
        cout << replacements << " occurrence(s) replaced.\n";
    }
    void undo() {
        string previous;
        if (!undoHistory.pop(previous)) {
            cout << "Nothing to undo.\n";
            return;
        }
        loadText(previous);
        changed = true;
        cout << "Last editing operation undone.\n";
    }
    void run() {
        int choice;
        do {
            cout << "\n========== TEXT EDITOR ==========\n"
                 << "1.  New document\n"
                 << "2.  Open file\n"
                 << "3.  Save\n"
                 << "4.  Save As\n"
                 << "5.  Display document\n"
                 << "6.  Document information\n"
                 << "7.  Insert text\n"
                 << "8.  Delete character\n"
                 << "9.  Delete word\n"
                 << "10. Delete text range\n"
                 << "11. Insert new line\n"
                 << "12. Delete line(s)\n"
                 << "13. Search text\n"
                 << "14. Replace one occurrence\n"
                 << "15. Replace all occurrences\n"
                 << "16. Undo\n"
                 << "0.  Exit\n";
            choice = readNumber("Enter choice: ");
            switch (choice) {
                case 1: newDocument(); break;
                case 2: openFile(); break;
                case 3: saveFile(); break;
                case 4: saveAs(); break;
                case 5: display(); break;
                case 6: showInformation(); break;
                case 7: insertText(); break;
                case 8: deleteCharacter(); break;
                case 9: deleteWord(); break;
                case 10: deleteRange(); break;
                case 11: insertNewLine(); break;
                case 12: deleteLines(); break;
                case 13: search(); break;
                case 14: replaceOne(); break;
                case 15: replaceAll(); break;
                case 16: undo(); break;
                case 0: cout << "Exiting Text Editor.\n"; break;
                case -1: cout << "Input ended. Exiting Text Editor.\n"; return;
                default: cout << "Invalid menu choice. Try again.\n";
            }
        } while (choice != 0);
    }
};
int main() {
    TextEditor editor;
    editor.run();
    return 0;
}
