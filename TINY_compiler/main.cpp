#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
#include <iostream>
#include <cmath>
using namespace std;

bool Equals(const char* a, const char* b)
{
    return strcmp(a, b)==0;
}

bool StartsWith(const char* a, const char* b)
{
    int nb=strlen(b);
    return strncmp(a, b, nb)==0;
}

void Copy(char* a, const char* b, int n = 0)
{
    if(n > 0)
    {
        strncpy(a, b, n);
        a[n] = 0;
    }
    else
        strcpy(a, b);
}

void AllocateAndCopy(char** a, const char* b)
{
    if(b == 0)
    {
        *a = 0;
        return;
    }
    int n = strlen(b);
    *a = new char[n+1];
    strcpy(*a, b);
}

#define MAX_LINE_LENGTH 10000

struct InFile
{
    FILE* file;
    int cur_line_num;

    char line_buf[MAX_LINE_LENGTH];
    int cur_ind, cur_line_size;

    InFile(const char* str)
    {
        file = 0;
        if(str)
            file = fopen(str, "r");
        cur_line_size = 0;
        cur_ind = 0;
        cur_line_num = 0;
    }
    ~InFile(){if(file) fclose(file);}

    void SkipSpaces()
    {
        while(cur_ind < cur_line_size)
        {
            char ch = line_buf[cur_ind];
            if(ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n')
                break;
            cur_ind++;
        }
    }

    bool SkipUpto(const char* str)
    {
        while(true)
        {
            SkipSpaces();
            while(cur_ind >= cur_line_size)
            {
                if(!GetNewLine())
                    return false;
                SkipSpaces();
            }

            if(StartsWith(&line_buf[cur_ind], str))
            {
                cur_ind+=strlen(str);
                return true;
            }
            cur_ind++;
        }
        return false;
    }

    bool GetNewLine()
    {
        cur_ind = 0;
        line_buf[0] = 0;
        if(!fgets(line_buf, MAX_LINE_LENGTH, file))
            return false;

        cur_line_size = strlen(line_buf);
        if(cur_line_size == 0)
            return false; // End of file

        cur_line_num++;
        return true;
    }

    char* GetNextTokenStr()
    {
        SkipSpaces();
        while(cur_ind >= cur_line_size)
        {
            if(!GetNewLine())
                return 0;
            SkipSpaces();
        }
        return &line_buf[cur_ind];
    }

    void Advance(int num) // num = the size of the comment
    {
        cur_ind += num;
    }
};

struct OutFile
{
    FILE* file;
    OutFile(const char* str) {file=0; if(str) file=fopen(str, "w");}
    ~OutFile(){if(file) fclose(file);}

    void Out(const char* s)
    {
        fprintf(file, "%s\n", s); fflush(file);
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Compiler Parameters /////////////////////////////////////////////////////////////

struct CompilerInfo
{
    InFile in_file;
    CompilerInfo(const char* in_str)
                : in_file(in_str)
    {
    }
};

////////////////////////////////////////////////////////////////////////////////////
// Scanner /////////////////////////////////////////////////////////////////////////

#define MAX_TOKEN_LEN 40

enum TokenType{
                IF, THEN, ELSE, END, REPEAT, UNTIL, READ, WRITE,
                ASSIGN, EQUAL, LESS_THAN,
                PLUS, MINUS, TIMES, DIVIDE, POWER,
                SEMI_COLON,
                LEFT_PAREN, RIGHT_PAREN,
                LEFT_BRACE, RIGHT_BRACE,
                ID, NUM,
                ENDFILE, ERROR
              };

const char* TokenTypeStr[]=
            {
                "If", "Then", "Else", "End", "Repeat", "Until", "Read", "Write",
                "Assign", "Equal", "LessThan",
                "Plus", "Minus", "Times", "Divide", "Power",
                "SemiColon",
                "LeftParen", "RightParen",
                "LeftBrace", "RightBrace",
                "ID", "Num",
                "EndFile", "Error"
            };

struct Token
{
    TokenType type;
    char str[MAX_TOKEN_LEN+1];

    Token(){str[0]=0; type=ERROR;}
    Token(TokenType _type, const char* _str) {type=_type; Copy(str, _str);}
};

const Token reserved_words[]=
{
    Token(IF, "if"),
    Token(THEN, "then"),
    Token(ELSE, "else"),
    Token(END, "end"),
    Token(REPEAT, "repeat"),
    Token(UNTIL, "until"),
    Token(READ, "read"),
    Token(WRITE, "write")
};

const int num_reserved_words = sizeof(reserved_words)/sizeof(reserved_words[0]);

// if there is tokens like < <=, sort them such that sub-tokens come last: <= <
// the closing comment should come immediately after opening comment
const Token symbolic_tokens[]=
{
    Token(ASSIGN, ":="),
    Token(EQUAL, "="),
    Token(LESS_THAN, "<"),
    Token(PLUS, "+"),
    Token(MINUS, "-"),
    Token(TIMES, "*"),
    Token(DIVIDE, "/"),
    Token(POWER, "^"),
    Token(SEMI_COLON, ";"),
    Token(LEFT_PAREN, "("),
    Token(RIGHT_PAREN, ")"),
    Token(LEFT_BRACE, "{"),
    Token(RIGHT_BRACE, "}")
};

const int num_symbolic_tokens = sizeof(symbolic_tokens)/sizeof(symbolic_tokens[0]);

inline bool IsDigit(char ch){return (ch>='0' && ch<='9');}
inline bool IsLetter(char ch){return ((ch>='a' && ch<='z') || (ch>='A' && ch<='Z'));}
inline bool IsLetterOrUnderscore(char ch){return (IsLetter(ch) || ch=='_');}

//The Scanner
void GetNextToken(CompilerInfo* compInfo, Token* ptoken)
{
    ptoken->type = ERROR;
    ptoken->str[0] = 0;

    int i;
    char* s = compInfo->in_file.GetNextTokenStr();
    if(!s)
    {
        ptoken->type = ENDFILE;
        ptoken->str[0] = 0;
        return;
    }

    for(i = 0; i < num_symbolic_tokens; i++)
    {
        if(StartsWith(s, symbolic_tokens[i].str))
            break;
    }

    //matching is done before the loop is done (entered the if condition )
    if(i < num_symbolic_tokens)
    {
        if(symbolic_tokens[i].type == LEFT_BRACE)  //{handling the comment}
        {
            compInfo->in_file.Advance(strlen(symbolic_tokens[i].str));
            if(!compInfo->in_file.SkipUpto(symbolic_tokens[i+1].str))
                return;

            return GetNextToken(compInfo, ptoken);
        }
        ptoken->type = symbolic_tokens[i].type;
        Copy(ptoken->str, symbolic_tokens[i].str);
    }
    else if(IsDigit(s[0]))
    {
        int j = 1;
        while(IsDigit(s[j]))
            j++;

        ptoken->type = NUM;
        Copy(ptoken->str, s, j);
    }
    else if(IsLetterOrUnderscore(s[0]))
    {
        int j = 1;
        while(IsLetterOrUnderscore(s[j]))
            j++;

        ptoken->type = ID;
        Copy(ptoken->str, s, j);

        for(i = 0; i < num_reserved_words; i++)
        {
            if(Equals(ptoken->str, reserved_words[i].str))
            {
                ptoken->type = reserved_words[i].type;
                break;
            }
        }
    }

    int len = strlen(ptoken->str);
    if(len > 0)
        compInfo->in_file.Advance(len);
}

////////////////////////////////////////////////////////////////////////////////////
// Parser //////////////////////////////////////////////////////////////////////////

// program -> stmtseq
// stmtseq -> stmt { ; stmt }
// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
// ifstmt -> if exp then stmtseq [ else stmtseq ] end
// repeatstmt -> repeat stmtseq until expr
// assignstmt -> identifier := expr
// readstmt -> read identifier
// writestmt -> write expr
// expr -> mathexpr [ (<|=) mathexpr ]
// mathexpr -> term { (+|-) term }    left associative
// term -> factor { (*|/) factor }    left associative
// factor -> newexpr { ^ newexpr }    right associative
// newexpr -> ( mathexpr ) | number | identifier

enum NodeKind{
                IF_NODE, REPEAT_NODE, ASSIGN_NODE, READ_NODE, WRITE_NODE,
                OPER_NODE, NUM_NODE, ID_NODE
             };

// Used for debugging only /////////////////////////////////////////////////////////
const char* NodeKindStr[]=
            {
                "If", "Repeat", "Assign", "Read", "Write",
                "Oper", "Num", "ID"
            };
enum ExprDataType {VOID, INTEGER, BOOLEAN};

const char* ExprDataTypeStr[]=
            {
                "Void", "Integer", "Boolean"
            };

#define MAX_CHILDREN 3

struct TreeNode
{
    TreeNode* child[MAX_CHILDREN];
    TreeNode* sibling; // used for sibling statements only

    NodeKind node_kind;

    union{TokenType oper; int num; char* id;}; // defined for expression/int/identifier only
    ExprDataType expr_data_type;
    int line_num;

    TreeNode()
    {
        int i;
        for(i = 0; i < MAX_CHILDREN; i++)
            child[i] = 0;

        sibling = 0;
        expr_data_type = VOID;
    }
};

struct ParseInfo
{
    Token next_token;
};


/// ********************* NEW ************************ ///
///prototypes
TreeNode* syntaxAnalysis(CompilerInfo*);
TreeNode* stmtSeq(CompilerInfo*, ParseInfo*);
TreeNode* stmt(CompilerInfo*, ParseInfo*);
TreeNode* ifStmt(CompilerInfo*, ParseInfo*);
TreeNode* repeatStmt(CompilerInfo*, ParseInfo*);
TreeNode* assignStmt(CompilerInfo*, ParseInfo*);
TreeNode* readStmt(CompilerInfo*, ParseInfo*);
TreeNode* writeStmt(CompilerInfo*, ParseInfo*);
TreeNode* expr(CompilerInfo*, ParseInfo*);
TreeNode* mathExpr(CompilerInfo*, ParseInfo*);
TreeNode* term(CompilerInfo*, ParseInfo*);
TreeNode* factor(CompilerInfo*, ParseInfo*);
TreeNode* newExpr(CompilerInfo*, ParseInfo*);
///-------------------------------------------------------///

// program -> stmtseq
TreeNode* syntaxAnalysis(const char* inputPath)
{
    ParseInfo parseInfo;
    CompilerInfo compInfo(inputPath);
    GetNextToken(&compInfo, &parseInfo.next_token);

    TreeNode* finalTree = stmtSeq(&compInfo, &parseInfo);
    return finalTree;
}


// stmtseq -> stmt {; stmt}
TreeNode* stmtSeq(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    //following the left-child-right sibling representation
    TreeNode* leftSubTree = stmt(compInfo, parseInfo);
    TreeNode* rightMostSubTree = leftSubTree;

    //if we have multiple statements
    while(true)
    {
        if(parseInfo->next_token.type == ELSE)
            return leftSubTree;

        if(parseInfo->next_token.type == UNTIL)
            return leftSubTree;

        if(parseInfo->next_token.type == END)
            return leftSubTree;

        if(parseInfo->next_token.type == ENDFILE)
            return leftSubTree;

        GetNextToken(compInfo, &parseInfo->next_token);     //advance to the next word
        //first statement (read x) | second statement (if .. end)
        TreeNode* nextSubTree = stmt(compInfo, parseInfo);   //gets the next statement / block (sub tree)
        rightMostSubTree->sibling = nextSubTree;
        rightMostSubTree = nextSubTree;
    }
    return leftSubTree;
}


// stmt -> ifstmt | repeatstmt | assignstmt | readstmt | writestmt
TreeNode* stmt(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    if(parseInfo->next_token.type == IF)
    {
        TreeNode* subTree = ifStmt(compInfo, parseInfo);
        return subTree;
    }
    else if(parseInfo->next_token.type == REPEAT)
    {
        TreeNode* subTree = repeatStmt(compInfo, parseInfo);
        return subTree;
    }
    else if(parseInfo->next_token.type == ID)  //assign
    {
        TreeNode* subTree = assignStmt(compInfo, parseInfo);
        return subTree;
    }
    else if(parseInfo->next_token.type == READ)
    {
        TreeNode* subTree = readStmt(compInfo, parseInfo);
        return subTree;
    }
    else if(parseInfo->next_token.type == WRITE)
    {
        TreeNode* subTree = writeStmt(compInfo, parseInfo);
        return subTree;
    }
    else
    {
        throw 0;
        return 0;
    }
}


// ifstmt -> if exp then stmtseq [ else stmtseq ] end
TreeNode* ifStmt(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = new TreeNode;
    subTree->node_kind = IF_NODE;
    subTree->line_num = compInfo->in_file.cur_line_num;

    //gets the subtree of the condition of the IF  (0<x)
    GetNextToken(compInfo, &parseInfo->next_token);
    subTree->child[0] = expr(compInfo, parseInfo);

    //gets the subtree of the body of the IF
    GetNextToken(compInfo, &parseInfo->next_token);
    subTree->child[1] = stmtSeq(compInfo, parseInfo);

    //if the IF statement has an ELSE statement, we consider the else child as one of the children of the IF
    if(parseInfo->next_token.type == ELSE)
    {
        GetNextToken(compInfo, &parseInfo->next_token);
        subTree->child[2] = stmtSeq(compInfo, parseInfo);
    }

    if(parseInfo->next_token.type == END)
    {
        GetNextToken(compInfo, &parseInfo->next_token);  //to skip the END
    }

    return subTree;
}


// repeatstmt -> repeat stmtseq until expr
TreeNode* repeatStmt(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = new TreeNode;
    subTree->node_kind = REPEAT_NODE;
    subTree->line_num = compInfo->in_file.cur_line_num;

    //gets the subtree of the body of the REPEAT
    GetNextToken(compInfo, &parseInfo->next_token);
    subTree->child[0] = stmtSeq(compInfo, parseInfo);

    //gets the subtree of the condition of the REPEAT  (x=0)
    GetNextToken(compInfo, &parseInfo->next_token);
    subTree->child[1] = expr(compInfo, parseInfo);

    return subTree;
}


// assignstmt -> identifier := expr
TreeNode* assignStmt(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = new TreeNode;
    subTree->node_kind = ASSIGN_NODE;
    subTree->line_num = compInfo->in_file.cur_line_num;

    if(parseInfo->next_token.type == ID)
    {
        AllocateAndCopy(&subTree->id, parseInfo->next_token.str);
        GetNextToken(compInfo, &parseInfo->next_token);    //gets the id
        GetNextToken(compInfo, &parseInfo->next_token);   //to skip the :=
        subTree->child[0] = expr(compInfo, parseInfo);
        return subTree;
    }
    else
    {
        throw 0;
        return 0;
    }
}


// readstmt -> read identifier
TreeNode* readStmt(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = new TreeNode;
    subTree->node_kind = READ_NODE;
    //subTree->expr_data_type = INTEGER;
    subTree->line_num = compInfo->in_file.cur_line_num;

    GetNextToken(compInfo, &parseInfo->next_token);  //gets the READ token
    if(parseInfo->next_token.type == ID)
    {
        AllocateAndCopy(&subTree->id, parseInfo->next_token.str);
        GetNextToken(compInfo, &parseInfo->next_token);
        return subTree;
    }
    else
    {
        throw 0;
        return 0;
    }
}


// writestmt -> write expr
TreeNode* writeStmt(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = new TreeNode;
    subTree->node_kind = WRITE_NODE;
    subTree->line_num = compInfo->in_file.cur_line_num;

    GetNextToken(compInfo, &parseInfo->next_token);
    subTree->child[0] = expr(compInfo, parseInfo);

    return subTree;
}


// expr -> mathexpr [ (<|=) mathexpr ]
TreeNode* expr(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = mathExpr(compInfo, parseInfo);

    //[the term inside the square bracket is optional]
    if(parseInfo->next_token.type == LESS_THAN || parseInfo->next_token.type == EQUAL)
    {
        TreeNode* newSubTree = new TreeNode;
        newSubTree->node_kind = OPER_NODE;
        newSubTree->expr_data_type = BOOLEAN;
        newSubTree->oper = parseInfo->next_token.type;
        newSubTree->line_num = compInfo->in_file.cur_line_num;

        newSubTree->child[0] = subTree;   //the LHS of the operator
        GetNextToken(compInfo, &parseInfo->next_token);
        newSubTree->child[1] = mathExpr(compInfo, parseInfo); //the RHS of the operator

        return newSubTree;
    }
    else
    {
        return subTree;
    }
}


// mathexpr -> term { (+|-) term }    left associative
TreeNode* mathExpr(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = term(compInfo, parseInfo);

    //the WHILE here is because {the term inside the curly brackets} is more likely to be repeated
    while(parseInfo->next_token.type == PLUS || parseInfo->next_token.type == MINUS)
    {
        TreeNode* newSubTree = new TreeNode;
        newSubTree->node_kind = OPER_NODE;
        newSubTree->expr_data_type = INTEGER;
        newSubTree->oper = parseInfo->next_token.type;
        newSubTree->line_num = compInfo->in_file.cur_line_num;

        newSubTree->child[0] = subTree;  //left operand
        GetNextToken(compInfo, &parseInfo->next_token);
        newSubTree->child[1] = term(compInfo, parseInfo);  //right operand

        subTree = newSubTree;
    }
    return subTree;
}


// term -> factor { (*|/) factor }    left associative
TreeNode* term(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = factor(compInfo, parseInfo);

    //the WHILE here is because {the term inside the curly brackets} is likely to be repeated
    while(parseInfo->next_token.type == TIMES || parseInfo->next_token.type == DIVIDE)
    {
        TreeNode* newSubTree = new TreeNode;
        newSubTree->node_kind = OPER_NODE;
        newSubTree->expr_data_type = INTEGER;
        newSubTree->oper = parseInfo->next_token.type;
        newSubTree->line_num = compInfo->in_file.cur_line_num;

        newSubTree->child[0] = subTree; //left operand
        GetNextToken(compInfo, &parseInfo->next_token);
        newSubTree->child[1] = factor(compInfo, parseInfo); //right operand

        subTree = newSubTree;
    }
    return subTree;
}


// factor -> newexpr { ^ newexpr } 2^3^1^5   right associative
TreeNode* factor(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    TreeNode* subTree = newExpr(compInfo, parseInfo); //2

    if(parseInfo->next_token.type == POWER)
    {
        TreeNode* newSubTree = new TreeNode;
        newSubTree->node_kind = OPER_NODE;
        newSubTree->expr_data_type = INTEGER;
        newSubTree->oper = parseInfo->next_token.type;  //^
        newSubTree->line_num = compInfo->in_file.cur_line_num;

        newSubTree->child[0] = subTree;     //left operand  2
        GetNextToken(compInfo, &parseInfo->next_token);
        //we use recursion to benefit from backtracking to calculate the power from right to left
        newSubTree->child[1] = factor(compInfo, parseInfo);  //right operand 3^1^5
        return newSubTree;
    }
    else
    {
        return subTree;
    }
}


// newexpr -> ( mathexpr ) | number | identifier   ex: (5+3) | 5 | x
TreeNode* newExpr(CompilerInfo* compInfo, ParseInfo* parseInfo)
{
    if(parseInfo->next_token.type == LEFT_PAREN)
    {
        GetNextToken(compInfo, &parseInfo->next_token); //(
        TreeNode* subTree = mathExpr(compInfo, parseInfo);

        GetNextToken(compInfo, &parseInfo->next_token); //skipping the )
        return subTree;
    }
    else if(parseInfo->next_token.type == NUM)
    {
        TreeNode* subTree = new TreeNode;
        subTree->node_kind = NUM_NODE;
        subTree->expr_data_type = INTEGER;
        //converting the char* to integer and store it in subTree->num
        char* numStr = parseInfo->next_token.str; //123
        string tempString = numStr;
        subTree->num = stoi(tempString);
        subTree->line_num = compInfo->in_file.cur_line_num;
        GetNextToken(compInfo, &parseInfo->next_token);
        return subTree;
    }
    else if(parseInfo->next_token.type == ID)
    {
        TreeNode* subTree = new TreeNode;
        subTree->node_kind = ID_NODE;
        subTree->expr_data_type = INTEGER;
        //store the value of the identifier (next_token.str ex:(xyz)) in the subtree->id
        AllocateAndCopy(&subTree->id, parseInfo->next_token.str);
        subTree->line_num = compInfo->in_file.cur_line_num;
        GetNextToken(compInfo, &parseInfo->next_token);
        return subTree;
    }
    else
    {
        throw 0;
        return 0;
    }
}


void printTree(TreeNode* node, int sh = 0)
{
    int i, NSH = 3;
    for(i = 0; i < sh; i++)
    {
        printf(" ");
    }

    printf("[%s]", NodeKindStr[node->node_kind]);

    if(node->node_kind == OPER_NODE)
    {
        printf("[%s]", TokenTypeStr[node->oper]);
    }
    else if(node->node_kind == NUM_NODE)
    {
        printf("[%d]", node->num);
    }
    else if(node->node_kind == ID_NODE || node->node_kind == READ_NODE || node->node_kind == ASSIGN_NODE)
    {
        printf("[%s]", node->id);
    }
    if(node->expr_data_type != VOID)
    {
        printf("[%s]", ExprDataTypeStr[node->expr_data_type]);
    }

    printf("\n");

    for(i = 0; i < MAX_CHILDREN; i++)
    {
        if(node->child[i])
        {
            printTree(node->child[i], sh+NSH);
        }
    }
    if(node->sibling)
    {
        printTree(node->sibling, sh);
    }
}


void DestroyTree(TreeNode* node)
{
    int i;

    if(node->node_kind==ID_NODE || node->node_kind==READ_NODE || node->node_kind==ASSIGN_NODE)
        if(node->id) delete[] node->id;

    for(i=0;i<MAX_CHILDREN;i++) if(node->child[i]) DestroyTree(node->child[i]);
    if(node->sibling) DestroyTree(node->sibling);

    delete node;
}


///  ////////////////////////////////////////////////////////////////
/// ///////////////////////////// NEW ////////////// NEW ///////////////////////////////////////
const int SYMBOL_HASH_SIZE = 10007;

struct LineLocation
{
    int line_num;
    LineLocation* next;
};


struct VariableInfo
{
    char* name;
    int memloc;
    LineLocation* head_line; // the head of linked list of source line locations
    LineLocation* tail_line; // the tail of linked list of source line locations
    VariableInfo* next_var; // the next variable in the linked list in the same hash bucket of the symbol table
};


struct SymbolTable
{
    int num_vars;
    VariableInfo* var_info[SYMBOL_HASH_SIZE];

    SymbolTable()
    {
        num_vars = 0;
        int i;
        for(i = 0; i < SYMBOL_HASH_SIZE; i++)
            var_info[i] = 0;
    }

    int Hash(const char* name)
    {
        int i, len = strlen(name);
        int hash_val = 11;
        for(i = 0; i < len; i++)
            hash_val = (hash_val*17 + (int)name[i]) % SYMBOL_HASH_SIZE;
        return hash_val;
    }

    VariableInfo* Find(const char* name)
    {
        int h = Hash(name);
        VariableInfo* cur = var_info[h];
        while(cur)
        {
            if(Equals(name, cur->name))
                return cur;
            cur = cur->next_var;
        }
        return 0;
    }

    void Insert(const char* name, int line_num)
    {
        LineLocation* lineloc = new LineLocation;
        lineloc->line_num = line_num;
        lineloc->next = 0;

        int h = Hash(name);
        VariableInfo* prev = 0;
        VariableInfo* cur = var_info[h];

        while(cur)
        {
            if(Equals(name, cur->name))
            {
                // just add this line location to the list of line locations of the existing var
                cur->tail_line->next = lineloc;
                cur->tail_line = lineloc;
                return;
            }
            prev = cur;
            cur = cur->next_var;
        }

        VariableInfo* vi = new VariableInfo;
        vi->head_line = vi->tail_line = lineloc;
        vi->next_var = 0;
        vi->memloc = num_vars++;
        AllocateAndCopy(&vi->name, name);

        if(!prev)
            var_info[h] = vi;
        else
            prev->next_var = vi;
    }

    void Print()
    {
        int i;
        for(i = 0; i < SYMBOL_HASH_SIZE; i++)
        {
            VariableInfo* curv = var_info[i];
            while(curv)
            {
                printf("[Var=%s][Mem=%d]", curv->name, curv->memloc);
                LineLocation* curl = curv->head_line;
                while(curl)
                {
                    printf("[Line=%d]", curl->line_num);
                    curl = curl->next;
                }
                printf("\n");
                curv = curv->next_var;
            }
        }
    }

    void Destroy()
    {
        int i;
        for(i = 0; i < SYMBOL_HASH_SIZE; i++)
        {
            VariableInfo* curv = var_info[i];
            while(curv)
            {
                LineLocation* curl = curv->head_line;
                while(curl)
                {
                    LineLocation* pl = curl;
                    curl = curl->next;
                    delete pl;
                }
                VariableInfo* p = curv;
                curv = curv->next_var;
                delete p;
            }
            var_info[i] = 0;
        }
    }
};

/// ////////////////////////////////////////////////
/// new ///////////////////////////////////////////

void typeChecking(TreeNode* node)
{
    if(node->node_kind == IF_NODE && node->child[0]->expr_data_type != BOOLEAN)
    {
        printf("============================================================================= \n");
        printf("Error!! invalid type for if-condition, condition has to be of type boolean \n");
        printf("============================================================================= \n");
    }

    if(node->node_kind == REPEAT_NODE && node->child[1]->expr_data_type != BOOLEAN)
    {
        printf("================================================================================= \n");
        printf("Error!! invalid type for repeat-condition, condition has to be of type boolean \n");
        printf("================================================================================= \n");
    }

    if(node->node_kind == ASSIGN_NODE && node->child[0]->expr_data_type != INTEGER)
    {
        printf("=========================================================================================== \n");
        printf("Error!! invalid type for the variable of the assign statement, integers only are allowed \n");
        printf("=========================================================================================== \n");
    }

    if(node->node_kind == WRITE_NODE && node->child[0]->expr_data_type != INTEGER)
    {
        printf("========================================================================================== \n");
        printf("Error!! invalid type for the variable of the write statement, integers only are allowed \n");
        printf("========================================================================================== \n");
    }

    if(node->node_kind == OPER_NODE  && (node->child[0]->expr_data_type != INTEGER || node->child[1]->expr_data_type != INTEGER))
    {
        printf("============================================================= \n");
        printf("Error!! invalid type for operand, integers only are allowed\n");
        printf("============================================================= \n");
    }
}


void buildSymbolTable(TreeNode* node, SymbolTable* symbol_table)
{
    int i;
    if(node->node_kind == ID_NODE || node->node_kind == READ_NODE || node->node_kind == ASSIGN_NODE)
    {
        symbol_table->Insert(node->id, node->line_num);
    }

    for(i = 0; i < MAX_CHILDREN; i++)
    {
        if(node->child[i])
        {
             buildSymbolTable(node->child[i], symbol_table);
        }
    }

    typeChecking(node);

    if(node->sibling)
    {
         buildSymbolTable(node->sibling, symbol_table);
    }
}


//runs the operations / evaluates the conditions / returns the variables
int run(TreeNode* node, SymbolTable* symbol_table, int* variables)
{
    if(node->node_kind == NUM_NODE)
    {
        int num = node->num;
        return num;
    }

    //assign / write / read
    if(node->node_kind == ID_NODE)
    {
        VariableInfo* varInfo = symbol_table->Find(node->id);
        int var = variables[varInfo->memloc];
        return var;
    }

    int leftChild, rightChild;
    leftChild = run(node->child[0], symbol_table, variables);
    rightChild = run(node->child[1], symbol_table, variables);

    if(node->oper == EQUAL)
    {
        int condition;
        if(leftChild == rightChild)
        {
            condition = 1;
        }
        else
        {
            condition = 0;
        }

        return condition;
    }

    else if(node->oper == LESS_THAN)
    {
        int condition;
        if(leftChild < rightChild)
        {
            condition = 1;
        }
        else
        {
            condition = 0;
        }

        return condition;
    }

    else if(node->oper == PLUS)
    {
        int result = leftChild + rightChild;
        return result;
    }

    else if(node->oper == MINUS)
    {
        int result = leftChild - rightChild;
        return result;
    }

    else if(node->oper == TIMES)
    {
        int result = leftChild * rightChild;
        return result;
    }

    else if(node->oper == DIVIDE)
    {
        int result = leftChild / rightChild;
        return result;

    }

    else if(node->oper == POWER)
    {
        return pow(leftChild, rightChild);
    }

    else
    {
        throw 0;
        return 0;
    }
}


//runs the if-statement / repeat-statement / assign-statement / read-statement / write-statement
void runCode(TreeNode* node, SymbolTable* symbolTable, int* memory)
{
    if(node->node_kind == IF_NODE)
    {
        //child[0] = the condition
        //child[1] = the body
        //child[2] = the else part body
        int condition = run(node->child[0], symbolTable, memory);

        // if the condition of the if-statement is true
        if(condition)
        {
            runCode(node->child[1], symbolTable, memory);
        }
        else if(node->child[2])
        {
            runCode(node->child[2], symbolTable, memory);
        }
    }

    else if(node->node_kind == REPEAT_NODE)
    {
        int condition;
        do
        {
           runCode(node->child[0], symbolTable, memory);
           condition = run(node->child[1], symbolTable, memory);
        }
        while(!condition);
    }

    else if(node->node_kind == ASSIGN_NODE)
    {
        int var = run(node->child[0], symbolTable, memory);
        VariableInfo* varInfo = symbolTable->Find(node->id);
        memory[varInfo->memloc] = var;
    }

    else if(node->node_kind == READ_NODE)
    {
        printf("Enter the value of %s: ", node->id);
        VariableInfo* varInfo = symbolTable->Find(node->id);
        scanf("%d", &memory[varInfo->memloc]);
    }

    else if(node->node_kind == WRITE_NODE)
    {
        int var = run(node->child[0], symbolTable, memory);
        printf("the value is: %d\n", var);
    }

    if(node->sibling)
    {
        runCode(node->sibling, symbolTable, memory);
    }
}


void codeGeneration(TreeNode* syntaxTree, SymbolTable* symbolTable)
{
    int i;
    int* memory = new int[symbolTable->num_vars];

    for(i = 0; i < symbolTable->num_vars; i++)
    {
       memory[i] = 0;
    }

    runCode(syntaxTree, symbolTable, memory);
    delete[] memory;
}

int main()
{
    string tempFilePath;
    cin >> tempFilePath;
    const char* filePath = tempFilePath.c_str();

    if(!fopen(filePath,"r"))
    {
        throw 0;
    }

    //parsing phase
    TreeNode* parseTree = syntaxAnalysis(filePath);
    printf("\nSyntax Tree:\n");
    printf("-------------\n");
    printTree(parseTree);
    printf("_________________________________________________________________\n\n");


    //generating the symbol table
    SymbolTable symbolTable;
    buildSymbolTable(parseTree, &symbolTable);
    printf("Symbol Table:\n");
    printf("--------------\n");
    symbolTable.Print();
    printf("_________________________________________________________________\n\n");


    //code generation phase
    printf("The run of the program:\n");
    printf("------------------------\n");
    codeGeneration(parseTree, &symbolTable);
    printf("__________________________________________________________________\n\n");

    DestroyTree(parseTree);
    symbolTable.Destroy();

    return 0;
}
