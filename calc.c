#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#define COUNT 2

#define DEBUG_MSG

#ifdef DEBUG_MSG
#define DMSG(...) fprintf(stderr,__VA_ARGS__)
#else
#define DMSG(...)
#endif


enum NodeType {OPERAND,OPERATOR};

struct Elem {
    void *data;
    struct Elem *next;
    struct Elem *perv;
};
typedef struct Elem Elem;

typedef struct  {
    Elem *top; 
    size_t size;
}Queue;

typedef struct {
    Elem *top;
    size_t size;
}Stack;

struct Node {
    enum NodeType type;
    union {double num;char sym;};
    struct Node *parent;
    struct Node *left;
    struct Node *right;
};
typedef struct Node Node;

typedef struct {
    Node *root;
    size_t size;
}Tree;


struct {
    Tree *main_tree ;
    Tree *curr_tree ;    
    Stack *treePoints_stack;
    Node *last_operator;
    Queue *operands_queue;
}ctx;

const char allowed_chars[] = "+-*/.=()^0123456789\n";

char isoperator(char );
char issign(char );
void ErrorCheck(const char*);
void ParseExp(const char*);
double SolveTree(Tree *);
void ExportDotFile(const char*,Tree*);
void print2DUtil(Node* root, int space);

void DestroyTree(Tree *t);

int main(int argc,char* argv[]){
    if(argc == 1){
        printf("Must provice an expression\n");
        return 1;
    }
    char *exp ;
    char *dot_file_name;
    char **args = argv;
    args ++;
    bool is_option = false;
    bool produce_dot = false;
    bool interactive = false;
    while(*args){
        char *arg = *args;
        if(arg[0] == '-'){
            if(produce_dot){
                fprintf(stderr,"%s is not a valid file name for the dot file\n",arg);
                exit(EXIT_FAILURE);
            }
            is_option = true;
            if(strcmp(arg,"--dot")==0){
                produce_dot = true;
            }
            if(strcmp(arg,"-i") == 0){
                interactive = true;
            }
        }else{
            if(is_option){
                if(produce_dot){
                   dot_file_name = arg; 
                }
            }else{
                exp = arg;
            }
        }
        args++;
    }
    size_t exp_size = 0; 
    do{
        ErrorCheck(exp);
        ParseExp(exp);
        print2DUtil(ctx.main_tree->root,0);
        double value = SolveTree(ctx.main_tree);
        printf("%f\n",value);
        if(produce_dot){
            ExportDotFile(dot_file_name,ctx.main_tree); 
        };
        if(interactive){
            getline(&exp,&exp_size,stdin);
        }
        DestroyTree(ctx.main_tree);
        ctx.main_tree = NULL;
    }while(interactive);

    return 0;
};

void ErrorCheck(const char *str){
    char* str_p = (char*)str;
    while(*str_p != '\0'){
        char *chr_p = (char*)allowed_chars;
        while(*chr_p != '\0'){
            if(*chr_p == *str_p)break; 
            chr_p++;
        }
        if(*chr_p != *str_p){
            int idx = str_p - str;  
            fprintf(stderr,"%s\n",str);
            fprintf(stderr,"%*c\n",idx+1,'^');
            fprintf(stderr,"Illegal character\n");
            exit(EXIT_FAILURE);
        }
        str_p++;
    }
};

char isoperator(char c){
    char *chr_p = (char*) allowed_chars;
    char delim = '.';
    while(*chr_p != delim){
        if(*chr_p == c)break; 
        chr_p++;
    }
    if(*chr_p == delim)return 0;
    return *chr_p;
}


char issign(char c){
    char *chr_p = (char*) allowed_chars;
    char delim = '*';
    while(*chr_p != delim){
        if(*chr_p == c)break; 
        chr_p++;
    }
    if(*chr_p == delim)return 0;
    return *chr_p;
}

Queue* CreateQueue(){
    DMSG("Creating New Queue\n");
    Queue *q = malloc(sizeof(Queue));
    q->top = NULL;
    q->size= 0;
    return q;
}


Stack* CreateStack(){
    DMSG("Creating New Stack\n");
    Stack *s = malloc(sizeof(Stack));
    s->top = NULL;
    s->size = 0;
    return s;
}

Tree* CreateTree(){
    DMSG("Creating New Tree\n");
    Tree *t = malloc(sizeof(Tree));
    t->root = NULL;
    t->size = 0;
    return t; 
}

Elem* CreateElem(void *data){
    DMSG("Creating New Elem\n");
    Elem *el = malloc(sizeof(Elem));
    el->data = NULL;
    el->next = NULL;
    el->perv = NULL;
    if(data != NULL)el->data = data;
    return el;
}



Node* CreateNode(enum NodeType type,double val,char sym){
    Node *n = malloc(sizeof(Node));
    DMSG("Creating New Node %p\n",n);
    n->num = 0;
    n->left = NULL;
    n->right= NULL;
    n->type = type;
    if(type == OPERAND){
        n->num = val;
    }else{
        n->sym = sym;
    }
    return n;
}

void EnQueue(Queue *q,Elem *el){
    q->size++;
    if(q->top == NULL){
        q->top = el;
        return;
    }
    Elem *last = q->top;
    while(last->perv != NULL){
        last = last->perv;
    }
    last->perv = el;
    el->next = last;

}

Elem* DeQueue(Queue *q){
    if(q->top == NULL){
        return NULL;
    }
    q->size--;
    Elem *el = q->top;
    q->top = el->perv;
    return el;
}

void* DeQueueData(Queue *q){
    Elem *el = DeQueue(q);
    if(el == NULL)return NULL;
    void *data = el->data;
    free(el);
    return data;
}

void Push(Stack *s,Elem *el){
    DMSG("Pushing Elem %p containing %p to Stack\n",el,el->data); 
    s->size++;
    if(s->top == NULL){
        s->top = el;
        return;
    }
    s->top->next = el;
    el->perv = s->top;
    s->top = el;
    return;
}

Elem* Pop(Stack *s){
    if(s->top == NULL){
        return NULL;
    }
    s->size--;
    Elem *el = s->top;
    DMSG("Poping Elem %p containing %p\n",el,el->data);
    s->top = s->top->perv;
    return el;
}

void* PopData(Stack *s){
    Elem *el = Pop(s);
    if(el == NULL)return NULL;
    void *data = el->data;
    free(el);
    return data;
}

bool IsChild(Tree* tree,Node *n){
    Node *root = n;
    while(n->parent != NULL){
        root = root->parent;
    }
    if(root != tree->root )return 0;
    return 1;
}

size_t CountNodes(Node* parent){
    if(parent == NULL)return 0;
    return 1+CountNodes(parent->left)+CountNodes(parent->right);
}

void TreeSize(Tree* tree){
    DMSG("Counting size of Tree %p\n",tree);
    Node *n = tree->root;
    size_t new_size = CountNodes(n);
    tree->size = new_size;
    DMSG("New Size = %ld\n",tree->size);
}

/**
 *  @side 0:left 1:right
 */
bool TreeAdd(Tree* tree,Node *parent,Node *child,unsigned int side){
    DMSG("++Adding Node %p at %p to Tree %p \n",child,parent,tree);
    if(parent == NULL){
        tree->root = child;
        tree->size++;
        return 1;
    }
    /*
    if(!IsChild(tree,parent)){
        fprintf(stderr,"Node not child of tree\n");
        exit(EXIT_FAILURE);
    }
    */
    if(!side){
        if(parent->left == NULL){
            parent->left = child;
            child->parent = parent;
        }
        else return 0;
    }else{
        if(parent->right == NULL){
            parent->right = child;
            child->parent = parent;
        }
        else return 0;
    }
        tree->size++;
        if(child->type == OPERATOR){
            ctx.last_operator = child;
        }
    return 1;
}

/**
 *  @side 0:left 1:right
 */
bool Graft(Tree *tree,Node *parent,Tree *branch,unsigned int side){
    DMSG("/\\Grafting Tree at %p with %p\n",parent,branch);
    /*
    if(!IsChild(tree,parent)){
        fDMSG(stderr,"Node not child of tree\n");
        exit(EXIT_FAILURE);
    }
    */
    Node *child = branch->root;
    if(!side){
        if(parent->left == NULL){
            parent->left = child;
            child->parent = parent;
        }
        else return 0;
    }else{
        if(parent->right == NULL){
            parent->right = child;
            child->parent = parent;
        }
        else return 0;
    }
    tree->size += branch->size;
    free(branch);
    return 1;
}

/**
 *  Return a New Tree(branch)
 */
Tree *Prune(Tree *tree,Node *new_root){
    DMSG("-\\Pruning Tree %p at %p\n",tree,new_root);

    /*
    if(!IsChild(tree,new_root)){
        fDMSG(stderr,"Node not child of tree\n");
        exit(EXIT_FAILURE);
    }
    */

    Node *parent = new_root->parent;
    if(parent->left == new_root){
        parent->left = NULL;
    }else{
        parent->right = NULL;
    }
    new_root->parent = NULL;

    Tree *new_tree = CreateTree();
    new_tree->root = new_root;
    TreeSize(new_tree);
    tree->size = tree->size - new_tree->size;
    
    return new_tree;
}

/**
 *  @side 0:left 1:right
 */
bool TreeSwap(Tree *tree,Node *new_parent,unsigned int side){
    DMSG("&Swapping Tree root %p for %p\n",tree->root,new_parent);
    if(!side){
        if(new_parent->left == NULL){
            new_parent->left = tree->root;
            tree->root->parent = new_parent;
        }
        else return 0;
    }else{
        if(new_parent->right == NULL){
            new_parent->right = tree->root;
            tree->root->parent = new_parent;
        }
        else return 0;
    }
    tree->root = new_parent;
    tree->size ++;
    return 1;
}


void FreeElem(Elem *el){
    free(el->data);
    free(el);
}

void DestroyQueue(Queue *q,void(*decon_f)(Elem*)){
    Elem *el = NULL;
    if(decon_f == NULL)decon_f = FreeElem;
    while((el = DeQueue(q)) != NULL){
        decon_f(el);
    }
    free(q);
}

void DestroyStack(Stack *s,void(*decon_f)(Elem*)){
    Elem *el = NULL;
    if(decon_f == NULL)decon_f = FreeElem;
    while((el = Pop(s)) != NULL){
        decon_f(el);
    }
    free(s);
}

void DestroyNodeChildren(Node *n){
    if(n->left != NULL){
        DestroyNodeChildren(n->left);
    }    
    if(n->right != NULL){
        DestroyNodeChildren(n->right);
    }
    free(n);
}


void DestroyTree(Tree *t){
    if(t == NULL)return;
    Node *n = t->root;
    DestroyNodeChildren(t->root);
    free(t);
}

typedef struct {
    Tree *t;
    Node *point;
}MergePoint;

MergePoint* CreateMergePoint(Tree *t,Node *p){
    MergePoint *mp = malloc(sizeof(MergePoint));
    mp->t = t;
    mp->point = p;
    DMSG("Creating merge Point: %p\n  Tree: %p\n  Point: %p\n",mp,mp->t,mp->point);
    return mp;
}

void FreeTreeElems(Elem *el){
    DestroyTree(((MergePoint*)el->data)->t); 
    free(((MergePoint*)el->data));
}

void FreeNodeElems(Elem *el){
    free(el->data);
    free(el);
}


void BeginTree(){
    Tree *t = CreateTree();
    DMSG("Starting Tree %p\n",t);
    MergePoint *mp = CreateMergePoint(ctx.curr_tree,ctx.last_operator);
    Push(ctx.treePoints_stack,CreateElem((void*)mp));
    ctx.curr_tree = t;
};

void EndTree(){
    
    MergePoint *mt = (MergePoint*) PopData(ctx.treePoints_stack);
    if(mt == NULL)return;
    if(mt->point == NULL){
        DMSG("Setting Main Tree %p\n",ctx.curr_tree);
        ctx.main_tree = ctx.curr_tree;
        free(mt);
        return;
    }
    Graft(mt->t,mt->point,ctx.curr_tree,1);
    ctx.last_operator = mt->point;
    ctx.curr_tree = mt->t;
    free(mt);
}

void BeginCtx(){
    ctx.main_tree = NULL;
    ctx.curr_tree = NULL;
    ctx.treePoints_stack = CreateStack();
    ctx.last_operator = NULL;
    ctx.operands_queue = CreateQueue();
};

void EndCtx(){
    for(int i=0;i<ctx.treePoints_stack->size;i++){
        EndTree();        
    }
    //DestroyTree(ctx.curr_tree);
    free(ctx.treePoints_stack);
    DestroyQueue(ctx.operands_queue,FreeNodeElems);
}

void AddOperand(double val){
    Tree *t = ctx.curr_tree;
    Node *n = CreateNode(OPERAND,val,0);
    DMSG("Adding Operand Node of val %f %p\n",val,n);
    if(t->size == 0){
        EnQueue(ctx.operands_queue,CreateElem(n));
        return;
    }
    TreeAdd(t,ctx.last_operator,n,1);
}


void InsertADD(Node *n){
    Tree *t = ctx.curr_tree;
    if(t->root == NULL){
        TreeAdd(t,NULL,n,0);
        if(ctx.operands_queue->size > 0){
            Node *queued_n = DeQueueData(ctx.operands_queue);
            TreeAdd(t,n,queued_n,0); 
            return;
        }
    }
    else{
        Node *root = t->root;
        if(root->sym == '*' || root->sym == '/'){
            EndTree();
        }
        TreeSwap(ctx.curr_tree,n,0);
    }
}
void InsertSUB(Node *n){
    Tree *t = ctx.curr_tree;
    if(t->root == NULL){
        TreeAdd(t,NULL,n,0);
        if(ctx.operands_queue->size > 0){
            Node *queued_n = DeQueueData(ctx.operands_queue);
            TreeAdd(t,n,queued_n,0); 
            return;
        }
    }
    else{
        Node *root = t->root;
        if(root->sym == '*' || root->sym == '/'){
            EndTree();
        }
        TreeSwap(ctx.curr_tree,n,0);
    }
}
void InsertMUL(Node *n){
    Tree *t = ctx.curr_tree;
    if(t->root == NULL ){
        TreeAdd(t,NULL,n,0);
        if(ctx.operands_queue->size > 0){
            Node *queued_n = DeQueueData(ctx.operands_queue);
            TreeAdd(t,n,queued_n,0); 
            return;
        }
        return;
    }
    if(t->root->sym == '*' || t->root->sym == '/'){
        TreeSwap(t,n,0); 
    }else{
        Tree *branch = NULL;
        if(ctx.last_operator->right != NULL){
           branch = Prune(t,ctx.last_operator->right);
        }
        BeginTree();
        t = ctx.curr_tree;
        TreeAdd(t,NULL,n,0);
        Graft(t,t->root,branch,0);
    }
}
void InsertDIV(Node *n){
    Tree *t = ctx.curr_tree;
    if(t->root == NULL ){
        TreeAdd(t,NULL,n,0);
        if(ctx.operands_queue->size > 0){
            Node *queued_n = DeQueueData(ctx.operands_queue);
            TreeAdd(t,n,queued_n,0); 
            return;
        }
        return;
    }
    if(t->root->sym == '*' || t->root->sym == '/'){
        TreeSwap(t,n,0); 
    }else{
        Tree *branch = NULL;
        if(ctx.last_operator->right != NULL){
           branch = Prune(t,ctx.last_operator->right);
        }
        BeginTree();
        t = ctx.curr_tree;
        TreeAdd(t,NULL,n,0);
        Graft(t,t->root,branch,0);
    }

}

void AddOperator(char sym){
    if(!isoperator(sym)){
        fprintf(stderr,"Invalid operator symbol\n");
        exit(EXIT_FAILURE);
    }
    Node *n = CreateNode(OPERATOR,0,sym);
    DMSG("Adding Operator Node %c %p\n",sym,n);
    switch(sym){
        case '+':
            InsertADD(n);
            break;
        case '-':
            InsertSUB(n);
            break;
        case '*':
            InsertMUL(n);
            break;
        case '/':
            InsertDIV(n);
            break;
    }
    ctx.last_operator = n;
}

void ParseExp(const char *str){
    char *str_p = (char*)str;
    enum NodeType parse_target = OPERAND;
    BeginCtx();
    BeginTree();
    while(*str_p != '\0' && *str_p != '\n'){
        if(*str_p == '('){
            BeginTree();
            /*
            if(parse_target == OPERATOR){
                AddOperator('*');
                parse_target = OPERAND;
            }
            */
        }
        else if(*str_p == ')'){
            EndTree();
        }
        else if(parse_target == OPERAND){
            if(isdigit(*str_p) || issign(*str_p)){
                double val = atof(str_p);
                AddOperand(val);
                parse_target = OPERATOR;
            }
        }
        else if(parse_target == OPERATOR){
            if(isoperator(*str_p)){
                AddOperator(*str_p);
                parse_target = OPERAND;
            }
        }
        str_p++;
    }
    EndTree();
    EndCtx();
}

void TraverseTree(Node *nd,void (*on_enter) (Node*)){
    on_enter(nd);
    if(nd->left != NULL){
        TraverseTree(nd->left,on_enter);
    }
    if(nd->right != NULL){
        TraverseTree(nd->right,on_enter);
    }
}

double SolveNode(Node *nd){
    double left_value = 0,right_value = 0;

    if(nd->left->type == OPERATOR){
        left_value = SolveNode(nd->left);
    }
    else{
        left_value = nd->left->num;  
    }
    if(nd->right->type == OPERATOR){
        right_value = SolveNode(nd->right);
    }
    else{
        right_value = nd->right->num;
    }
    switch(nd->sym){
            case '+':
                return left_value + right_value;
                break;
            case '-':
                return left_value - right_value;
                break;
            case '*':
                return left_value * right_value;
                break;
            case '/':
                return left_value / right_value;
                break;
    }

    return 0; 

}

double SolveTree(Tree *tree){
    return SolveNode(tree->root); 
}

FILE* fd = NULL;
Node **node_array = NULL;

const char IDs[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char* id_pointer = (char*) IDs;

void PrintNodes(Node *nd){
    if(*id_pointer == '\0')return ;
    node_array[id_pointer-IDs] = nd;
    DMSG("Attached Node %p to Key %c || %ld\n",nd,*id_pointer,id_pointer-IDs);
    char text[64] = {'\0'};
    char name[32] = {'\0'};
    if(nd->type == OPERAND){
        sprintf(name,"%.0f",nd->num);
    }
    else if(nd->type == OPERATOR){
        sprintf(name,"%c",nd->sym);
    }
    size_t count = sprintf(text,"   %c [label=\"%s\"]\n",*id_pointer,name);
    fwrite(text,1,count,fd);  

    id_pointer++;
}

char GetID(Node *nd){
    char id = 0;
    for(int i=0;i<sizeof(IDs);i++){
        if(node_array[i] == nd){
            id = IDs[i]; 
            DMSG("Matched Node %p to ID %c\n",nd,id);
            break;
        }
    }
    return id;
}

void ConnectNodes(Node *nd){
    char text[64] = {'\0'};
    char id = GetID(nd);
    char l_id,r_id;
    l_id = (nd->left != NULL)? GetID(nd->left) : 0;
    r_id = (nd->right != NULL)? GetID(nd->right) : 0;
    if(l_id){
        int count = sprintf(text,"  %c -> %c\n",id,l_id);
        fwrite(text,1,count,fd);  
    }
    if(r_id){
        int count = sprintf(text,"  %c -> %c\n",id,r_id);
        fwrite(text,1,count,fd);  
    }

}

void ExportDotFile(const char *file_name,Tree *tree){
    fd = fopen(file_name,"w");
    node_array = malloc(sizeof(Node*)*tree->size); 
    id_pointer = (char*)IDs;
    fprintf(fd,"digraph Exp {\n");

    TraverseTree(tree->root,PrintNodes);
    TraverseTree(tree->root,ConnectNodes);

    fprintf(fd,"}\n");

    fclose(fd);
    free(node_array);

    
}



void print2DUtil(Node* root, int space)
{
    // Base case
    if (root == NULL)
        return;
 
    // Increase distance between levels
    space += COUNT;
 
    // Process right child first
    print2DUtil(root->right, space);
 
    // Print current node after space
    // count
    DMSG("\n");
    for (int i = COUNT; i < space; i++)
        DMSG(" ");
    if(root->type == OPERAND){
        DMSG("%f\n", root->num);
    }
    else if(root->type == OPERATOR){
        DMSG("%c\n", root->sym);
    }
 
    // Process left child
    print2DUtil(root->left, space);
}
