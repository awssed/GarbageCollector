#include <vector>
#include<iostream>
using namespace std;
enum { SIZE = 0, REFER_COUNT = 1 };
struct Header {
    union {
        int Data[2];
        Header* post_Add;
    };
};

vector<Header**> referencesStack;

template <class T>
class stackRef {
public:
    T* ref;
    stackRef() {
        ref = nullptr;
        referencesStack.push_back(reinterpret_cast<Header**>(&ref));
    }

    ~stackRef() {
        referencesStack.pop_back();
    }
};
const int BLOCK_SIZE = 4096;
const int OVER = 128;
const int FULL_SIZE = BLOCK_SIZE ;
struct Block {
    Block* next;
    char data[FULL_SIZE];
};

Block* firstBlock;
Block* currentBlock;
int blockCount;
int currentOffset;
Header* RawAlloc(int size, int refCount) {
    if (size > BLOCK_SIZE)
        return nullptr;
    if (currentOffset + size > BLOCK_SIZE) { 
        ++blockCount;
        currentBlock->next = new Block();
        currentBlock = currentBlock->next;
        currentBlock->next = nullptr;
        currentOffset = 0;
    }
    Header* new_obj = reinterpret_cast<Header*>(&(currentBlock->data[currentOffset]));
    new_obj->Data[SIZE] = size;
    new_obj->Data[REFER_COUNT] = (refCount << 1) | 1;
    currentOffset += size;
    if (currentOffset % 4)
        currentOffset += 4 - currentOffset % 4;
    return new_obj;
}
template <class T>
T* Alloc() {
    return reinterpret_cast<T*>(RawAlloc(sizeof(T), T::refCount));
}
void Init() {
    firstBlock = currentBlock = new Block;
    firstBlock->next = nullptr;
    currentOffset = 0;
    blockCount = 1;
}
bool isPointer(Header a) {
    return (a.Data[REFER_COUNT] & 1) == 0;
}
void Move(Header** current) {
    if (*current == nullptr)
        return;
    if (isPointer(**current)) {
        (*current) = (*current)->post_Add;
        return;
    }
    Header* new_obj = RawAlloc((*current)->Data[SIZE], (*current)->Data[REFER_COUNT]);
    memcpy(new_obj, (*current), sizeof(char) * (*current)->Data[SIZE]);

    Header** iterator = reinterpret_cast<Header**>(new_obj) + 1;


    (*current)->post_Add = new_obj;
    (*current) = new_obj;
    int refCount = new_obj->Data[REFER_COUNT] >> 1;
    for (int i = 0; i < refCount; ++i, ++iterator)
        Move(iterator);
}
void Collect() {
    Block* newFirstBlock = currentBlock = new Block;
    currentBlock->next = nullptr;
    currentOffset = 0;
    blockCount = 1;
    for (auto i = referencesStack.begin(); i != referencesStack.end(); ++i)
        Move(*i);
    Block *iter = firstBlock;
    firstBlock = newFirstBlock;
    while (iter != nullptr) {
        Block* t = iter->next;
        delete[] iter;
        iter = t;
    }
}
struct searchTree {
    Header gc;
    searchTree* left;
    searchTree* right;
    int key;
    static const int refCount = 2;
};
void stAdd(searchTree*& target, int key) {
    if (target == nullptr) {

        target = Alloc<searchTree>();
        target->left = target->right = nullptr;
        target->key = key;

        return;
    }
    if (target->key == key)
        return;
    if (target->key < key)
        stAdd(target->left, key);
    else
        stAdd(target->right, key);
}


void stPrint(searchTree* t, int indent = 0) {
    if (t == nullptr)
        return;
    for (int i = 0; i < indent; ++i)
        cout << "  ";
    cout << t << ' ' << t->key << endl;
    stPrint(t->left, indent + 1);
    stPrint(t->right, indent + 1);

}


void stCut(searchTree*& target, int key) {
    if (target == nullptr || target->key == key) {
        target = nullptr;
        return;
    }
    if (target->key < key)
        stCut(target->left, key);
    else
        stCut(target->right, key);
}

int main() {
    setlocale(LC_ALL, "rus");
    Init();
    stackRef<searchTree> root;

    stAdd(root.ref, 2);
    stAdd(root.ref, 1);
    stAdd(root.ref, 3);
    stAdd(root.ref, 6);
    stAdd(root.ref, 5);
    stAdd(root.ref, 4);
    stAdd(root.ref, 8);
    stackRef<searchTree> additionalRef;
    cout << "Состояние до работы сборщика" << endl;
    cout << additionalRef.ref << ' ' << currentOffset << endl << endl;
    stPrint(root.ref);
    cout << endl;

    Collect();
    cout << "Состояние после работы сборщика" << endl;
    cout << additionalRef.ref << ' ' << currentOffset << endl << endl;
    stPrint(root.ref);

    cout << endl;
    stCut(root.ref, 5);
    Collect();
    cout << "Удалён элементы и проведена сборка мусора" << endl;
    cout << additionalRef.ref << ' ' << currentOffset << endl << endl;
    stPrint(root.ref);

    return 0;


}