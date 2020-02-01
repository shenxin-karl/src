#include <iostream>
#include <cstdlib>
#include <ctime> 
#include "RBTree.hpp" 
#include <unistd.h>
#include <time.h>


int main()
{
    RBTree<int> rbtree;

    srand(time(nullptr));

    for (int i = 0; i < 99; ++i) 
        rbtree.insert(rand() % 200);

    auto end = rbtree.end();
    for (auto it = rbtree.begin(); it != end; ++it) 
        std::cout << *it << " ";

    printf("\n");
    printf(" ");
}
