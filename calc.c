#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

enum nodetype {
    OPERAND =  1 << 0,
    OPERATOR=  1 << 1
};

enum Operators  {
    ADD = 1 ,
    SUB = 2 ,
    MUL = 3 ,
    DIV = 4 
};


struct node{
    struct node *parent;
    enum nodetype type;
    union{
        double d;
        enum Operators op;
    }value;
    struct node *children[2]; 
};
typedef struct node tree;

struct lnode{
    void *v;
    struct lnode *next;
    struct lnode *perv;
};
typedef struct lnode stack;
typedef struct lnode queue;

struct context{
    stack *treeStack; 
    queue *operatorQueue;
};

struct context *current_context = NULL;

 

struct node *root = NULL;

double operands_stack [256] = {0};
int num_of_operands = 0;
const char allowed_chars[] = "+-*/().0123456789";
const char operators_chars[] = "+-*/()";
size_t precedence_level = 0;

int SyntaxCheck(char * str);
void Parser(char* str);
void PrintTree(tree *t);

int main(int argc,char* argv[]){

    if(argc > 1){
        if(SyntaxCheck(argv[1])){
            Parser(argv[1]);
            PrintTree(current_context->treeStack->v); 
        };
    }
    else{
        printf("Please pass an expression\n");
    }
    

    return 0;
}


void HighlightError(int idx){
    for(int i=1;i<idx+1;i++){
        putc(' ',stderr);  
    }
    fprintf(stderr,"^\n");  
}

int SyntaxCheck(char * str){

    char* str_p = str;
    bool errored = false;
    char* opendParenthesis[32] = {NULL};
    while(*str_p != '\0'){
        for(const char *allowed = &allowed_chars[0];*allowed != '\0';allowed++){
            if(*str_p == *allowed){
                if(*str_p == '('){
                    precedence_level++;
                    opendParenthesis[precedence_level-1] = str_p;

                }
                else if (*str_p == ')'){
                    if(precedence_level == 0 ){
                        errored = true;
                        int idx = str_p - str;
                        fprintf(stderr,"Closing Parentheses Before Opening at %d \n",idx);
                        fprintf(stderr,"%s\n",str);
                        HighlightError(idx);
                         
                    }
                    else{
                        precedence_level--;
                        opendParenthesis[precedence_level] = NULL;
                    }
                }
                break;
            }
            if(*allowed == allowed_chars[sizeof(allowed_chars)-2]){
                errored = true;
                int idx = str_p - str;
                fprintf(stderr,"illegal Character %c at %d \n",*str_p,idx);
                fprintf(stderr,"%s\n",str);
                HighlightError(idx);
            }
        }
        str_p++;

    }
    if(precedence_level > 0){
        errored = true;
        for(int i=0;i<precedence_level;i++){
            int idx = opendParenthesis[i] - str;
            fprintf(stderr,"Unclosed Parentheses at %d \n",idx);
            fprintf(stderr,"%s\n",str);
            HighlightError(idx);
        }

    };
    return !errored;
}

const char * IsOperator(char c){
    for(const char *operators_p = &operators_chars[0];*operators_p != '\0';operators_p++){
        if(c == *operators_p){
            //printf("Matched Operator %c \n",c);
            return operators_p;
        }
    };
 return NULL;
};

enum Operators GetOperator(const char * op){

    return (enum Operators)strcspn(operators_chars,op)+1;

}




double PopOperand(){
    if(num_of_operands == 0){
        fprintf(stderr,"Poping empty operands stack");
        exit(EXIT_FAILURE);
    }
    double op = operands_stack[num_of_operands];
    num_of_operands--;
    return op;
};

struct node* CreateNode(enum nodetype nt){
    struct node *nd = (struct node*) malloc(sizeof(struct node));
    nd->parent = NULL;
    nd->type = nt;
    nd->children[0] = NULL;
    nd->children[1] = NULL;
    nd->value.d = 0.0f;
    return nd;
};

void CopyNode(struct node *dest,struct node *src){
    memcpy(dest,src,sizeof(struct node));
};

void PrintNode(struct node *nd){
    char str[128] = {'\0'};
    if(nd->type == OPERATOR){
        sprintf(str,"%c",operators_chars[nd->value.op-1]);
    }else{
        sprintf(str,"%.6g",nd->value.d);
    }
    printf("ADDR: %p\nTYPE: %s\nVALUE: %s\nPARENT: %p\nLEFT: %p\nRIGHT: %p\n",nd,(nd->type == OPERATOR)?"OPERATOR":"OPERAND",str,nd->parent,nd->children[0],nd->children[1]);

}

void AddChild(struct node *nd,struct node *child,size_t idx){
    if(idx > 1){
        fprintf(stderr,"children index can't be over 1\n");
        exit(EXIT_FAILURE);
    }
    if(nd->children[idx] != NULL){
        fprintf(stderr,"child can't be inserted, already occupied\n");
        exit(EXIT_FAILURE);
    };
    printf("Adding %p to %p at %s\n",child,nd,(idx)?"RIGHT":"LEFT");
    nd->children[idx] = child;
    child->parent = nd;

}

void AddParent(struct node *nd,struct node *parent,size_t idx){
    if(idx > 1){
        fprintf(stderr,"children index can't be over 1\n");
        exit(EXIT_FAILURE);
    }
    if(parent->children[idx] != NULL){
        fprintf(stderr,"child can't be inserted,Parent already occupied\n");
        exit(EXIT_FAILURE);
    };
    nd->parent = parent;
    AddChild(parent,nd,idx);
}

struct lnode* CreateListElem(void* val){
    struct lnode *nd = (struct lnode*) malloc(sizeof(struct lnode));
    nd->v = (val != NULL)? val : NULL;
    nd->next = NULL;
    nd->perv = NULL;
    return nd;
}

void Enqueue(queue **queue,struct lnode *nd){
    //printf("Adding to queue %p\n",nd->v);
    if(*queue == NULL){
        *queue = nd;
        return;
    }
    struct lnode* last = (*queue);
    while(last->perv != NULL){
        last = last->perv;
    }
    nd->next = last;
    last->perv = nd;

}

struct lnode* Dequeue(queue **queue){
    if(*queue == NULL)return NULL;
    struct lnode *lnd = *queue;
    *queue = (*queue)->perv;
    if((*queue) != NULL){
        (*queue)->next = NULL;
    }
    lnd->perv = NULL;
    return lnd;
};

void PrintQueue(queue *queue){
    struct lnode *lnd = queue;
    size_t counter=0;
    printf("queue element #%ld v:%p\n",counter,lnd->v);
    while(lnd->perv != NULL){
        counter++;
        lnd = lnd->perv;
        printf("queue element #%ld v:%p\n",counter,lnd->v);
    }


};

void PushStack(stack **stack,struct lnode *nd){
    if(*stack == NULL){
        *stack = nd;
        return;
    }
    nd->next = *stack;
    (*stack)->perv = nd;
    (*stack) = nd;
};

struct lnode* PopStack(stack **stack){
    if(*stack == NULL)return NULL;
    struct lnode *lnd = *stack;
    *stack = (*stack)->next;
    if((*stack) != NULL){
        (*stack)->perv = NULL;
    }
    lnd->next = NULL;
    return lnd;
};

void DestroyStack(stack *stack,void (*decon)(void* elem)){
    //printf("Destroying Stack\n");
    if(stack == NULL)return;
    struct lnode *last = (struct lnode*)stack;
    while(last->next != NULL){
        last = last ->next;
        if(decon != NULL){
            decon(last->perv->v);
        }
        free(last->perv);
    };
    if(decon != NULL){
        decon(last->v);
    }
    free(last);
    return;
};

void DestroyTree(tree *tree){
    printf("Destroying Tree\n");
    if(tree == NULL)return;
    struct node *nd = (struct node*) tree;
    if(nd->children[0] != NULL){
        DestroyTree(nd->children[0]);
    };
    if(nd->children[1] != NULL){
        DestroyTree(nd->children[1]);
    };
    free(nd);

};

typedef struct{
    size_t depth;
    long width_left;
    size_t num_nodes;
}treeInfo;

treeInfo GetTreeInfo(tree *t){
    treeInfo info = {.depth = 0,.width_left = 0,.num_nodes = 1};
    int l=0,d=0;
    size_t idx=0;
    struct node *nd = t;
    stack *stack = NULL;
    while(nd != NULL || stack != NULL){
        while(nd != NULL){
            PushStack(&stack,CreateListElem(nd));
            nd = nd->children[0];
            if(nd != NULL){
                //printf("Left\n");
                //printf("%p \n",nd);
                d++;
                l++;
                info.depth = (d > info.depth)? d : info.depth;
                info.width_left = (l > info.width_left)? l : info.width_left;
            info.num_nodes++;
            }
        }
        struct lnode *top = PopStack(&stack);
        nd = (top == NULL)? NULL : top->v;
        free(top);
        nd = (nd == NULL)? NULL : nd->children[1];
        d--;
        l--;
        if(nd != NULL){
            d++;
            info.depth = (d > info.depth)? d : info.depth;
            info.num_nodes++;
        }

    };
    printf("LEFT %ld\n",info.width_left);
    DestroyStack(stack,NULL);
    return info;
};

typedef struct pa_str{
    unsigned int abs_offset;
    struct pa_str* next;
    char str[];
} pa_str;

pa_str* CreatePaStr(const char str[],unsigned int offset){
    pa_str *pastr = malloc(sizeof(pa_str) + sizeof(char) * strlen(str));
    pastr->abs_offset = offset;
    strcpy(pastr->str,str);
    pastr->next = NULL;
    return pastr;
};

void InsertPaStr(pa_str *pa,pa_str *ch){
    pa_str *last = pa;
    printf("%p",last);
    while(last->next != NULL){
        last = last->next;
        printf("->%p",last);
    }
    putchar('\n');
    last->next = ch;
};



void PrintTree(tree *t){
    treeInfo info = GetTreeInfo(t);
    int node_v_offset = 1,node_h_offset = 1;
    unsigned int add_offset = 0;
    pa_str* strings[info.num_nodes];
    for(int i=0;i<info.num_nodes;i++){
        strings[i] = NULL;
    }
    size_t num_strings = 0;
    size_t x = 0,y = 0;
    int start = 0;
    struct node *nd = t;
    queue *queues[2] = {NULL};
    int  cq = 0;
    Enqueue(&(queues[cq]),CreateListElem(nd));
    
    while(nd != NULL || queues[0] != NULL || queues[1] != NULL){
        if(queues[cq] == NULL){
            cq = !cq;
            y++;
            x = 0;
        }
        struct lnode *top = Dequeue(&(queues[cq]));
        nd = (top == NULL)?NULL : top->v;
        free(top);
        if(nd != NULL){
            if(nd->children[0] != NULL){
                x++;
                Enqueue(&(queues[!cq]),CreateListElem(nd->children[0]));
            }
            if(nd->children[1] != NULL){
                x++;
                Enqueue(&(queues[!cq]),CreateListElem(nd->children[1]));
            }
            char str[128] = {'\0'};
            size_t len = 0;
            switch(nd->type){
                case OPERATOR:
                    len = sprintf(str,"%c",operators_chars[nd->value.op-1]); 
                    
                    if(strings[y] == NULL){
                        strings[y] = CreatePaStr(str,len+x+(info.width_left-y-1));
                        num_strings++;
                    }else{
                        InsertPaStr(strings[y],CreatePaStr(str,len+x));
                    }
                    
                    break;
                case OPERAND:
                    len = sprintf(str,"%.6g",nd->value.d); 
                    double len_half = round((double)len/2);
                    if(len_half > info.width_left+x){
                        int offset = len_half -  info.width_left+x;
                        len += offset;
                        for(int i=num_strings-1;i==0;i--){
                            pa_str *last = strings[i]; 
                            while(last->next != NULL){
                                last->abs_offset += offset-1;
                                last = last->next;
                            }
                        }
                    }
                    if(strings[y] == NULL){
                        num_strings++;
                        strings[y] = CreatePaStr(str,len+x+(info.width_left-y));
                    }else{
                        InsertPaStr(strings[y],CreatePaStr(str,len+x));
                    }

                    break;

            }
        }
        else{

        }
    };
    printf("Width: %ld, Depth: %ld, Nodes: %ld\n",info.width_left,info.depth,info.num_nodes); 
    for(int i=0;i<num_strings;i++){
        pa_str *last = strings[i]; 
        int idx = 1;
        printf("%*c",last->abs_offset-strlen(last->str),' ');
        printf("%s",last->str);
        while(last->next != NULL){
            last = last->next;
            printf("%*c",last->abs_offset-strlen(last->str)-idx,' ');
            printf("%s",last->str);
        }
        putchar('\n');
    }
}
void BeginTree();
void EndTree();

void BeginCtx(){
    
    printf("Begin CTX\n");
    struct context *ctx = (struct context*) malloc(sizeof(struct context));
    ctx->treeStack = NULL;
    ctx->operatorQueue = NULL;
    current_context = ctx;   
    BeginTree();
};


void Decon(void* elem){
   DestroyTree((tree*) elem);  
}
void DestroyCtx(struct context* ctx){
    printf("Destroy CTX\n");
    if(ctx == NULL)return;
    //DestroyStack(ctx->treeStack,Decon);
    //DestroyTree(ctx->currentSubTree);
    free(ctx);
};


tree* EndCtx(){
    printf("End CTX\n");
    struct context *ctx = current_context;
    EndTree();
    //DestroyCtx(ctx);    
    return ctx->treeStack->v;
};

struct node* GetLastOperatorNode(tree* t,size_t idx){
    struct node *nd = t;
    if(t->children[idx] != NULL && t->children[idx]->type & OPERATOR){
        nd = t->children[idx];
        nd = GetLastOperatorNode(nd,idx);
    }
    return nd;
};

void BeginTree(){
    struct context *ctx = current_context;
    printf("Begin Tree\n");
    PushStack(&ctx->treeStack,CreateListElem(NULL));
    Dequeue(&(ctx->operatorQueue));


};

void EndTree(){
    struct context *ctx = current_context;
    if(ctx->treeStack == NULL){
       fprintf(stderr,"Called End Tree before and BeginTree\n");
       exit(EXIT_FAILURE);
    }
    printf("End Tree\n");
    struct lnode *t = PopStack(&(ctx->treeStack)) ;
    struct lnode *tmp = ctx->treeStack; 
    /*
    printf("Merging\n");
    PrintTree(t->v);
    printf("To\n");
    PrintTree(tmp->v);
    */

    if(tmp == NULL){
        PushStack(&(ctx->treeStack),t);
        return;
    }else{
        AddChild((tree*)tmp->v,t->v,1);
    }

}
void AddOperator(struct node *nd){
    printf("Adding Operator\n");
    if(nd->type != OPERATOR){
        fprintf(stderr,"Node passed as OPERATOR NODE but is NOT\n");
        exit(EXIT_FAILURE);
    };
    printf("value OP = %d, value c = %c\n",nd->value.op,operators_chars[nd->value.op-1]);
    struct context *ctx = current_context;
    
    if(ctx->treeStack->v == NULL){
        printf("Added Root");
        ctx->treeStack->v = nd;
        //if Root insert to queue twice
        Enqueue(&(ctx->operatorQueue),CreateListElem(ctx->treeStack->v));
        Enqueue(&(ctx->operatorQueue),CreateListElem(ctx->treeStack->v));
        return;
    }
    else{
        printf("Root %p\n",ctx->treeStack->v);
    }
    struct lnode *tmp = ctx->treeStack;
    tree *root = ctx->treeStack->v;
    printf("Added Operator %p\n",nd);
    switch(nd->value.op){
        case MUL:
        case DIV:
            if(root->value.op == MUL || root->value.op == DIV){
                Enqueue(&(ctx->operatorQueue),CreateListElem(nd));
                AddParent(root,nd,0);
                ctx->treeStack->v = nd;
            }else{
                Dequeue(&(ctx->operatorQueue));
                BeginTree();
                AddOperator(nd);
            }
            break;

        case ADD:
        case SUB:
            if(root->value.op == MUL || root->value.op == DIV ){
                EndTree();
            }
            Enqueue(&(ctx->operatorQueue),CreateListElem(nd));
            AddParent(root,nd,0);
            ctx->treeStack->v = nd;
            break;


    };

}

void AddOperand(struct node *nd){
    printf("Adding Operand\n");
    printf("value OP = %d, value c = %.6g\n",nd->value.op,nd->value.d);
    if(nd->type != OPERAND){
        fprintf(stderr,"Node passed as OPERAND NODE but is NOT\n");
        exit(EXIT_FAILURE);
    };
    struct context *ctx = current_context;
    struct lnode *top = Dequeue(&ctx->operatorQueue);
    struct node *parent = (top == NULL)?NULL : top->v;
    if(parent == NULL){
        
    }
    if(parent->children[0] == NULL){
        AddChild(parent,nd,0);
    }
    else if(parent->children[1] == NULL){
        AddChild(parent,nd,1);
    }else{
        fprintf(stderr,"No Place to Place OPERAND NODE\n");
        printf("---Node---\n");
        PrintNode(nd);
        printf("---Parent---\n");
        PrintNode(parent);
        exit(EXIT_FAILURE);
    };


}




typedef struct  {
    double operand;
    enum Operators operator;
}slice;





void Parser(char* str){
    printf("%s what??\n",str);
    char* ptr = str;
    BeginCtx();
    while(true){
        ptr = strpbrk(str,operators_chars);
        ptr = (ptr == NULL)? &str[strlen(str)] : ptr;
        printf("%c\n",*ptr);
        double operand_v = 0.0f;
        enum Operators operator_v = 0;
        int span = ptr - str;
        if(*ptr == '('){
            BeginTree();
            if(span > 0){
                struct node *operator_n = CreateNode(OPERATOR); 
                operator_n->value.op = MUL;
                AddOperator(operator_n);
            }
        }
        else if(*ptr == ')'){
            EndTree();
            char next_char = *(ptr+1);
            if(next_char != '\0' && !IsOperator(next_char)){
                struct node *operator_n = CreateNode(OPERATOR); 
                operator_n->value.op = MUL;
                AddOperator(operator_n);
            }
        }
        else if(*ptr != '\0'){
            enum Operators operator_v = GetOperator(ptr);
            printf("ENUM %d\n",operator_v);
            struct node *operator_n = CreateNode(OPERATOR); 
            operator_n->value.op = operator_v;
            AddOperator(operator_n);
        }

        if(span>0){
            operand_v = atof(str); 
            struct node *operand_n = CreateNode(OPERAND); 
            operand_n->value.d = operand_v;
            AddOperand(operand_n);
        }
        if(*ptr == '\0')break;
        str = ptr+1;
        printf("\n");
    }
    EndCtx();
    exit(EXIT_SUCCESS);
}

