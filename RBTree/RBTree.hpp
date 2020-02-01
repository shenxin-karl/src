#ifndef _RBTREE_HPP_
#define _RBTREE_HPP_
#include <utility>
#include <exception>


/* 
 * 类功能:      red-black tree 红黑树
 * 提供中序遍历只读迭代器类型 
 */
template<typename T> 
class RBTree {
    enum RBColor : bool { RED, BLACK };         /* 定义枚举颜色表示结点颜色 */
    struct RBNode;                              /* 树结点嵌套类声明 */
    using size_type = unsigned int;            
public: 
    class Iterator;                             /* 迭代器嵌套类声明 */ 
private:
    RBNode      *m_root;                        /* 根节点指针 */
    RBNode      *m_nil;                         /* 哨兵空节点 */
    size_type    m_size;                        /* 树的结点数量 */
private:

    /* 函数说明:    左旋修复操作, 修正红黑树的平衡
     * @root:       需要旋转的根结点
     */
    void left_rotate(RBNode *root)
    {
        RBNode *newRoot = root->right;

        root->right = newRoot->left;
        if (root->right != m_nil)
            root->right->parent = root;

        newRoot->parent = root->parent;
        if (root->parent == m_nil)
            m_root = newRoot;
        else if (root == root->parent->left)
            root->parent->left = newRoot;
        else 
            root->parent->right = newRoot;

        newRoot->left = root;
        root->parent = newRoot;
    }

    /*
     * 函数说明:    右旋转修复操作, 修正红黑树的平衡
     * @root        需要旋转的根结点 
     */
    void right_rotate(RBNode *root)
    {
        RBNode *newRoot = root->left;

        root->left = newRoot->right;
        if (root->left != m_nil)
            root->left->parent = root;

        newRoot->parent = root->parent;
        if (root->parent == m_nil)
            m_root = newRoot;
        else if (root == root->parent->left)
            root->parent->left = newRoot;
        else 
            root->parent->right = newRoot;

        newRoot->right = root;
        root->parent = newRoot;
    }


    /*
     * 函数说明:    替换一个结点
     * @root:       树结点
     * @newRoot:    替换的结点
     */
    void transplant(RBNode *root, RBNode *newRoot)
    {
        if (root->parent == m_nil)
            m_root = newRoot;
        else if (root == root->parent->left)
            root->parent->left = newRoot;
        else 
            root->parent->right = newRoot;

        newRoot->parent = root->parent;
    }

    /* 
     * 函数说明:    给定一个键值, 寻找对应键值的结点
     * 返回值:      如果存在返回 保存 key 的结点指针, 否则返回 m_nil
     * @key:        键值
     */
    RBNode *search_node(T const &key)
    {
        RBNode *pNode = m_root;

        while (pNode != m_nil) {
            if (key == pNode->key)
                return pNode;
            else if (key < pNode->key)
                pNode = pNode->left;
            else 
                pNode = pNode->right;
        }

        return pNode;
    }

    /*
     * 函数说明:    给定一个树根, 返回树中的最小 key 键值的结点指针
     * @root:       树根指针
     */
    RBNode *min(RBNode *root) 
    {
        while (root != m_nil && root->left != m_nil)
            root = root->left;

        return root;
    }

    /*
     * 同上, const this 版本
     */
    RBNode const *min(RBNode *root) const 
    {
        while (root != m_nil && root->left != m_nil)
            root = root->left;
        
        return root;
    }

    /*
     * 函数说明:    当插入结点成功时, 调用 insert_fixup 对红黑树进行修正, 使红黑树满足性质
     */
    void insert_fixup(RBNode *node)
    {
        RBNode *parent;
        RBNode *gparent;
        RBNode *uncle;

        while (node->parent->color == RED) {
            parent = node->parent;
            gparent = parent->parent;

            /* 父结点在祖父结点的左边 */
            if (parent == gparent->left) {
                uncle = gparent->right;
                /* 情况1 */
                if (uncle->color == RED) {
                    uncle->color = BLACK;
                    parent->color = BLACK;
                    gparent->color = RED;
                    node = gparent;
                } else {
                    /* 情况2 */
                    if (node == parent->right) {
                        node = parent;
                        left_rotate(node);
                    }

                    /* 情况3 */
                    node->parent->color = BLACK;
                    node->parent->parent->color = RED;
                    right_rotate(node->parent->parent);
                } 

                /* 父结点在祖父结点的右边 */
            } else {
                uncle = gparent->left;
                /* 情况1 */
                if (uncle->color == RED) {
                    uncle->color = BLACK;
                    node->parent->color = BLACK;
                    gparent->color = RED;
                    node = gparent;
                } else {
                    /* 情况2 */
                    if (node == parent->left) {
                        node = parent;
                        right_rotate(node);
                    }

                    /* 情况3 */
                    node->parent->color = BLACK;
                    node->parent->parent->color = RED;
                    left_rotate(node->parent->parent);
                }
            }
        } /* while */

        m_root->color = BLACK;
    }

    /* 
     * 函数说明:    给定一个结点指针, 在红黑树中删除该结点
     * @node:       需要删除的结点指针
     */
    void remove(RBNode *node)
    {
        RBNode *original = node;
        RBColor originalColor = node->color;
        RBNode *rplNode;

        /* 删除结点只有右孩子 */
        if (node->left == m_nil) {
            rplNode = node->right;
            transplant(node, node->right);

            /* 删除结点只有左孩子 */
        } else if (node->right == m_nil) {
            rplNode = node->left;
            transplant(node, node->left);

            /* 删除结点有两个孩子 */
        } else {
            original = min(node->right);
            originalColor = original->color;
            rplNode = original->right;

            if (original->parent == node)
                rplNode->parent = original;
            else {
                transplant(original, original->right);
                original->right = node->right;
                original->right->parent = original;
            }

            transplant(node, original);
            original->left = original->left;
            original->left->parent = original;
            original->color = node->color;
        }

        if (originalColor == BLACK)
            remove_fixup(rplNode);
    }

    /* 
     * 函数说明:    删除修正操作, 当删减的结点是红色时, 需要调用 remove_fixup 调整红黑树
     * @node:       被删除或者替换的结点
     */
    void remove_fixup(RBNode *node)
    {
        RBNode *brother;

        while (node != m_root && node->color == BLACK) {
            /* 当前结点在父节点的左边 */
            if (node == node->parent->left) {
                brother = node->parent->right;
                /* 情况1 */
                if (brother->color == RED) {
                    brother->color = BLACK;
                    node->parent->color = RED;
                    left_rotate(node->parent);
                    brother = node->parent->right;
                }

                /* 情况2 */
                if (brother->left->color == BLACK && brother->right->color == BLACK) {
                    brother->color = RED;
                    node = node->parent;
                } else {
                    /* 情况3 */
                    if (brother->right->color == BLACK) {
                        brother->color = RED;
                        brother->left->color = BLACK;
                        right_rotate(brother);
                        brother = node->parent->right;
                    }
                    /* 情况4 */
                    brother->color = node->parent->color;
                    brother->right->color = BLACK;
                    node->parent->color = BLACK;
                    left_rotate(node->parent);
                    node = m_root;
                }

                /* 当前节点在父节点的右边 */
            } else {
                brother = node->parent->left;
                /* 情况1 */
                if (brother->color == RED) {
                    brother->color = BLACK;
                    node->parent->color = RED;
                    right_rotate(node->parent);
                    brother = node->parent->left;
                }

                /* 情况2 */
                if (brother->left->color == BLACK && brother->right->color == BLACK) {
                    brother->color = RED;
                    node = node->parent;
                } else {
                    /* 情况3 */
                    if (brother->left->color == BLACK) {
                        brother->color = RED;
                        brother->right->color = BLACK;
                        left_rotate(brother);
                        brother = node->parent->left;
                    }
                    /* 情况4 */
                    brother->color = node->parent->color;
                    brother->left->color = BLACK;
                    node->parent->color = BLACK;
                    right_rotate(node->parent);
                    node = m_root;
                }
            }
        }

        node->color = BLACK;
    }

    /* 
     * 函数说明:    递归的销毁红黑树
     * @root:       红黑树根结点
     */
    void destroy(RBNode *root)
    {
        if (root == m_nil)
            return;

        if (root->left != m_nil)
            destroy(root->left);

        if (root->right != m_nil)
            destroy(root->right);

        delete root;
    }

    /* 
     * 函数说明:    拷贝红黑树
     * 返回值:      返回拷贝的红黑树根结点
     * @root:       需要拷贝的红黑树
     * @otherNil:   需要拷贝的红黑树的哨兵结点指针
     * @nil:        新的红黑树指向的哨兵结点
     */
    static RBNode *tree_copy(RBNode const *root, RBNode const * const otherNil, RBNode const * const nil)
    {
        if (root == otherNil)
            return nil;

        RBNode *newNode = new RBNode(root->key, nullptr, nil, root->color);

        newNode->left = tree_copy(root->left, otherNil, nil);
        newNode->right = tree_copy(root->right, otherNil, nil);

        if (newNode->left != nil)
            newNode->left->parent = newNode;

        if (newNode->right != nil)
            newNode->right->parent = newNode;

        return newNode;
    }

public:
    /* 默认构造 */
    RBTree() : m_size(0)
    {
        m_nil = new RBNode(T(), nullptr, nullptr, BLACK);

        m_nil->parent = m_nil;
        m_nil->left = m_nil;
        m_nil->right = m_nil;
        m_root = m_nil;
    }

    /* 析构函数 */
    ~RBTree()
    {
        destroy(m_root);
        delete m_nil;
    }

    /* 拷贝构造 */
    RBTree(RBTree const &other) : m_size(other.size)
    {
        m_nil = new RBNode(T(), nullptr, nullptr, BLACK);

        m_nil->parent = m_nil;
        m_nil->left = m_nil;
        m_nil->right = m_nil;

        m_root = tree_copy(other.m_root, other.m_nil, m_nil);
        m_root->parent = m_nil;
    }

    /* 移动构造 */
    RBTree(RBTree &&other) : m_root(other.m_root), m_nil(other.m_nil), m_size(other.m_size)
    {
        other.m_size = 0;
        other.m_nil = new RBNode(T(), nullptr, nullptr, BLACK);

        other.m_nil->parent = other.m_nil;
        other.m_nil->left = other.m_nil;
        other.m_nil->right = other.m_nil;

        other.m_root = other.m_nil;
    }

    /* 赋值操作符 */
    RBTree &operator=(RBTree const &other) 
    {
        if (this == &other)
            return *this;

        RBTree tmp(other);
        swap(*this, tmp);

        return *this;
    }

    /* 移动赋值操作符 */
    RBTree &operator=(RBTree &&other)
    {
        if (this == &other)
            return *this;

        RBTree tmp(std::move(other));
        swap(*this, tmp);

        return *this;
    }

    /* 
     * 函数说明:    给定一个 键值 key 插入一个新结点到红黑树中
     * 返回值:      成功返回 true, 失败返回 false 
     * @key:        新插入红黑树中的键值
     */
    bool insert(T const &key)
    {
        RBNode *pNode = m_nil;
        RBNode *curNode = m_root;

        while (curNode != m_nil) {
            pNode = curNode;

            if (key == curNode->key)
                return false;
            else if (key < curNode->key)
                curNode = curNode->left;
            else 
                curNode = curNode->right;
        }

        RBNode *newNode = new RBNode(key, pNode, m_nil);
        if (pNode == m_nil)
            m_root = newNode;
        else if (key < pNode->key)
            pNode->left = newNode;
        else 
            pNode->right = newNode;

        insert_fixup(newNode);
        ++m_size;

        return true;
    }

    /*
     * 函数说明:    删除红黑树中的 key 键值
     * @key:        键值
     */
    bool remove(T const &key)
    {
        RBNode *node;

        if ((node = search_node(key)) != m_nil) {
            remove(node);
            --m_size;
            return true;
        }

        return false;
    }

    /*
     * 函数说明:    返回红黑树中的结点数量
     */
    size_type size() const
    {
        return m_size;
    }

    /* 
     * 函数说明:    判断红黑树是否为空
     */
    bool empty() const
    {
        return size() == 0;
    }

    /*
     * 函数说明:    自定义 swap 函数, 提高交换效率 
     */
    friend void swap(RBTree &first, RBTree &second)
    {
        std::swap(first.m_root, second.m_root);
        std::swap(first.m_nil, second.m_nil);
        std::swap(first.m_size, second.m_size); 
    }

    /*
     * 函数说明:    返回红黑树的开始迭代器
     */
    Iterator begin() const 
    {
        return Iterator(min(m_root), m_nil);
    }

    /*
     * 函数说明:    返回红黑树的尾后迭代器 
     */
    Iterator end() const 
    {
        return Iterator(m_nil, m_nil);
    }
};


/* 红黑树嵌套结点类 */
template<typename T>
struct RBTree<T>::RBNode {
    struct RBNode   *parent;        /* 指向父结点 */
    struct RBNode   *left;          /* 指向左孩子 */
    struct RBNode   *right;         /* 指向右孩子 */
    enum   RBColor   color;         /* 结点颜色 */
    T                key;           /* 保存的键值 */
public:
    RBNode(T const &key, RBNode *p, RBNode *nil, RBColor color = RED)
        : key(key), parent(p), left(nil), right(nil), color(color) {  }

    ~RBNode() = default;
};



class IvalidIterator : public std::exception {
};

class NullIterator : public std::exception {
};


/*
 * 类说明:  红黑树迭代器类 
 */
template<typename T>
class RBTree<T>::Iterator {
private:
    RBNode  const *m_curNode;
    RBNode  const *m_nil;
private:

    void check() const
    {
        if (m_curNode == nullptr && m_nil == nullptr)
            throw NullIterator();

        if (m_curNode == m_nil)
            throw IvalidIterator();
    }

public:

    Iterator() : m_curNode(nullptr), m_nil(nullptr)
    {}

    explicit Iterator(RBNode const *curNode, RBNode const *nil) : m_curNode(curNode), m_nil(nil)
    { }

    ~Iterator() = default;
    Iterator(Iterator const &other) = default;

    Iterator &operator++() 
    {
        check();

        T const &original = m_curNode->key;
        if (m_curNode->right != m_nil) {
            m_curNode = m_curNode->right;
            while (m_curNode->left != m_nil)
                m_curNode = m_curNode->left;

        } else {
            m_curNode = m_curNode->parent;
            while (m_curNode->key < original && m_curNode != m_nil)
                m_curNode = m_curNode->parent;
        }

        return *this;
    }

    Iterator operator++(int)
    {
        Iterator ret = *this;

        ++(*this);
        return ret;
    }

    T const &operator*() const 
    {
        check();

        return m_curNode->key;
    }

    T const *operator->() const 
    {
        return &this->operator*();
    }

    Iterator &operator--() 
    {
        check();

        T const &original = m_curNode->key;
        if (m_curNode->left != m_nil) {
            m_curNode = m_curNode->left;
            while (m_curNode->right != m_nil)
                m_curNode = m_curNode->right;

        } else {
            while (m_curNode->key >= original && m_curNode != m_nil)
                m_curNode = m_curNode->parent;

            check();
        }

        return *this;
    }

    Iterator operator--(int)
    {
        Iterator ret = *this;
        --(*this);
        return ret;
    }

    friend Iterator operator+(Iterator &iter,int n)
    {
        Iterator ret = iter;
        
        if (n > 0) {
            for (int i = 0; i < n; ++i)
                ++ret;
        } else {
            for (int i = 0; i < (-n); ++i)
                --ret;
        }

        return ret;
    }

    friend Iterator operator-(Iterator &iter, int n)
    {
       Iterator ret = iter;

       if (n > 0) {
           for (int i = 0; i < n; ++i)
               --ret;
       } else {
           for (int i = 0; i < (-n); ++i)
               ++ret;
       }

       return ret;
    }

    Iterator &operator=(Iterator const &other)
    {
        this->m_curNode = other.m_curNode;
        this->m_nil = other.m_nil;

        return *this;
    }

    Iterator &operator+=(int n)
    {
        if (n > 0) {
            for (int i = 0; i < n; ++i)
                ++(*this);
        } else {
            for (int i = 0; i < (-n); ++i)
                --(*this);
        }

        return *this;
    }

    Iterator &operator-=(int n)
    {
        if (n > 0) {
            for (int i = 0; i < n; ++i)
                --(*this);
        } else {
            for (int i = 0; i < (-n); ++i)
                ++(*this);
        }

        return *this;
    }

    friend void swap(Iterator &first, Iterator &second)
    {
        std::swap(first.m_curNode, second.m_curNode);
        std::swap(first.m_nil, second.m_nil);
    }

    friend bool operator==(Iterator const &first, Iterator const &second)
    {
        if (first.m_curNode == nullptr || second.m_nil == nullptr)
            return false;

        if (first.m_nil != second.m_nil)
            return false;

        return (first.m_curNode == second.m_curNode);
    }

    friend bool operator!=(Iterator const &first, Iterator const &second)
    {
        return !(first == second);
    }
};




#endif


