
#if defined(_WIN32)
    #include <fcntl.h>
    #include <io.h>
#endif

#if !defined(NEON_PLAT_ISWASM) && !defined(NEON_PLAT_ISWINDOWS)
    #define NEON_USE_LINENOISE
#endif

#if defined(NEON_USE_LINENOISE)
    #include "linenoise.h"
#endif


#if defined(__STRICT_ANSI__)
    #define va_copy(...)
#endif

#if defined(__GNUC__)
    #define NEON_LIKELY(x) \
        __builtin_expect(!!(x), 1)

    #define NEON_UNLIKELY(x) \
        __builtin_expect(!!(x), 0)
#else
    #define NEON_LIKELY(x) x
    #define NEON_UNLIKELY(x) x
#endif

/* needed when compiling with wasi. must be defined *before* signal.h is included! */
#if defined(__wasi__)
    #define _WASI_EMULATED_SIGNAL
#endif
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

/*
* needed because clang + wasi (wasi headers, specifically) don't seem to define these.
* note: keep these below stdlib.h, so as to check whether __BEGIN_DECLS is defined.
*/
#if defined(__wasi__) && !defined(__BEGIN_DECLS)
    #define __BEGIN_DECLS
    #define __END_DECLS
    #define __THROWNL
    #define __THROW
    #define __nonnull(...)
#endif

#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>


#if defined(_WIN32) || defined(_WIN64)
    #include <sys/utime.h>
    #define NEON_PLAT_ISWINDOWS
#else
    #if defined(__wasi__)
        #define NEON_PLAT_ISWASM
    #endif
    #include <sys/time.h>
    #include <utime.h>
    #include <dirent.h>
    #include <dlfcn.h>
    #include <unistd.h>
    #define NEON_PLAT_ISLINUX
#endif

#include "strbuf.h"
#include "optparse.h"
#include "os.h"


#if defined(__GNUC__) || defined(__clang__)
    #define nn_util_likely(x)   (__builtin_expect(!!(x), 1))
    #define nn_util_unlikely(x) (__builtin_expect(!!(x), 0))
#else
    #define nn_util_likely(x)   (x)
    #define nn_util_unlikely(x) (x)
#endif

#if defined(__STRICT_ANSI__)
    #define NEON_INLINE static
    #define NEON_FORCEINLINE static
    #define inline
#else
    #define NEON_INLINE static inline
    #if defined(__GNUC__)
        #define NEON_FORCEINLINE static __attribute__((always_inline)) inline
    #else
        #define NEON_FORCEINLINE static inline
    #endif
#endif

#define MT_STATE_SIZE 624

#define NEON_CFG_FILEEXT ".nn"

/* global debug mode flag */
#define NEON_CFG_BUILDDEBUGMODE 0

#if NEON_CFG_BUILDDEBUGMODE == 1
    #define DEBUG_PRINT_CODE 1
    #define DEBUG_TABLE 0
    #define DEBUG_GC 1
    #define DEBUG_STACK 0
#endif

/* initial amount of frames (will grow dynamically if needed) */
#define NEON_CFG_INITFRAMECOUNT (32)

/* initial amount of stack values (will grow dynamically if needed) */
#define NEON_CFG_INITSTACKCOUNT (32 * 1)

/* how many locals per function can be compiled */
#define NEON_CFG_ASTMAXLOCALS 64

/* how many upvalues per function can be compiled */
#define NEON_CFG_ASTMAXUPVALS 64

/* how many switch cases per switch statement */
#define NEON_CFG_ASTMAXSWITCHCASES 32

/* max number of function parameters */
#define NEON_CFG_ASTMAXFUNCPARAMS 32

/* how deep template strings can be nested (i.e., "foo${getBar("quux${getBonk("...")}")}") */
#define NEON_CFG_ASTMAXSTRTPLDEPTH 8

/* how many catch() clauses per try statement */
#define NEON_CFG_MAXEXCEPTHANDLERS 16

/*
// Maximum load factor of 12/14
// see: https://engineering.fb.com/2019/04/25/developer-tools/f14/
*/
#define NEON_CFG_MAXTABLELOAD 0.85714286

/* how much memory can be allocated before the garbage collector kicks in */
#define NEON_CFG_DEFAULTGCSTART (1024 * 1024)

/* growth factor for GC heap objects */
#define NEON_CFG_GCHEAPGROWTHFACTOR 1.25

#define NEON_INFO_COPYRIGHT "based on the Blade Language, Copyright (c) 2021 - 2023 Ore Richard Muyiwa"

#if defined(__GNUC__)
    #define NEON_ATTR_PRINTFLIKE(fmtarg, firstvararg) __attribute__((__format__(__printf__, fmtarg, firstvararg)))
#else
    #define NEON_ATTR_PRINTFLIKE(a, b)
#endif

/*
// NOTE:
// 1. Any call to nn_gcmem_protect() within a function/block must be accompanied by
// at least one call to nn_gcmem_clearprotect() before exiting the function/block
// otherwise, expected unexpected behavior
// 2. The call to nn_gcmem_clearprotect() will be automatic for native functions.
// 3. $thisval must be retrieved before any call to nn_gcmem_protect in a
// native function.
*/
#define GROW_CAPACITY(capacity) \
    ((capacity) < 4 ? 4 : (capacity)*2)

#define nn_gcmem_growarray(state, typsz, pointer, oldcount, newcount) \
    nn_gcmem_reallocate(state, pointer, (typsz) * (oldcount), (typsz) * (newcount))

#define nn_gcmem_freearray(state, typsz, pointer, oldcount) \
    nn_gcmem_release(state, pointer, (typsz) * (oldcount))

#define NORMALIZE(token) #token

#define nn_exceptions_throwclass(state, exklass, ...) \
    nn_exceptions_throwwithclass(state, exklass, __FILE__, __LINE__, __VA_ARGS__)

#define nn_exceptions_throw(state, ...) \
    nn_exceptions_throwclass(state, state->exceptions.stdexception, __VA_ARGS__)

#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32_MAJOR_VERSION)
    #include <libgen.h>
    #include <limits.h>
#endif

#define NEON_RETURNERROR(...) \
    { \
        nn_vm_stackpopn(state, args->count); \
        nn_exceptions_throw(state, ##__VA_ARGS__); \
    } \
    return nn_value_makebool(false);

#define NEON_ARGS_FAIL(chp, ...) \
    nn_argcheck_fail((chp), __FILE__, __LINE__, __VA_ARGS__)

/* check for exact number of arguments $d */
#define NEON_ARGS_CHECKCOUNT(chp, d) \
    if(nn_util_unlikely((chp)->argc != (d))) \
    { \
        return NEON_ARGS_FAIL(chp, "%s() expects %d arguments, %d given", (chp)->name, d, (chp)->argc); \
    }

/* check for miminum args $d ($d ... n) */
#define NEON_ARGS_CHECKMINARG(chp, d) \
    if(nn_util_unlikely((chp)->argc < (d))) \
    { \
        return NEON_ARGS_FAIL(chp, "%s() expects minimum of %d arguments, %d given", (chp)->name, d, (chp)->argc); \
    }

/* check for range of args ($low .. $up) */
#define NEON_ARGS_CHECKCOUNTRANGE(chp, low, up) \
    if(nn_util_unlikely((chp)->argc < (low) || (chp)->argc > (up))) \
    { \
        return NEON_ARGS_FAIL(chp, "%s() expects between %d and %d arguments, %d given", (chp)->name, low, up, (chp)->argc); \
    }

/* check for argument at index $i for $type, where $type is a nn_value_is*() function */
#define NEON_ARGS_CHECKTYPE(chp, i, type) \
    if(nn_util_unlikely(!type((chp)->argv[i]))) \
    { \
        return NEON_ARGS_FAIL(chp, "%s() expects argument %d as " NORMALIZE(type) ", %s given", (chp)->name, (i) + 1, nn_value_typename((chp)->argv[i])); \
    }

enum NNOpCode
{
    NEON_OP_GLOBALDEFINE,
    NEON_OP_GLOBALGET,
    NEON_OP_GLOBALSET,
    NEON_OP_LOCALGET,
    NEON_OP_LOCALSET,
    NEON_OP_FUNCARGSET,
    NEON_OP_FUNCARGGET,
    NEON_OP_UPVALUEGET,
    NEON_OP_UPVALUESET,
    NEON_OP_UPVALUECLOSE,
    NEON_OP_PROPERTYGET,
    NEON_OP_PROPERTYGETSELF,
    NEON_OP_PROPERTYSET,
    NEON_OP_JUMPIFFALSE,
    NEON_OP_JUMPNOW,
    NEON_OP_LOOP,
    NEON_OP_EQUAL,
    NEON_OP_PRIMGREATER,
    NEON_OP_PRIMLESSTHAN,
    NEON_OP_PUSHEMPTY,
    NEON_OP_PUSHNULL,
    NEON_OP_PUSHTRUE,
    NEON_OP_PUSHFALSE,
    NEON_OP_PRIMADD,
    NEON_OP_PRIMSUBTRACT,
    NEON_OP_PRIMMULTIPLY,
    NEON_OP_PRIMDIVIDE,
    NEON_OP_PRIMFLOORDIVIDE,
    NEON_OP_PRIMMODULO,
    NEON_OP_PRIMPOW,
    NEON_OP_PRIMNEGATE,
    NEON_OP_PRIMNOT,
    NEON_OP_PRIMBITNOT,
    NEON_OP_PRIMAND,
    NEON_OP_PRIMOR,
    NEON_OP_PRIMBITXOR,
    NEON_OP_PRIMSHIFTLEFT,
    NEON_OP_PRIMSHIFTRIGHT,
    NEON_OP_PUSHONE,
    /* 8-bit constant address (0 - 255) */
    NEON_OP_PUSHCONSTANT,
    NEON_OP_ECHO,
    NEON_OP_POPONE,
    NEON_OP_DUPONE,
    NEON_OP_POPN,
    NEON_OP_ASSERT,
    NEON_OP_EXTHROW,
    NEON_OP_MAKECLOSURE,
    NEON_OP_CALLFUNCTION,
    NEON_OP_CALLMETHOD,
    NEON_OP_CLASSINVOKETHIS,
    NEON_OP_RETURN,
    NEON_OP_MAKECLASS,
    NEON_OP_MAKEMETHOD,
    NEON_OP_CLASSGETTHIS,
    NEON_OP_CLASSPROPERTYDEFINE,
    NEON_OP_CLASSINHERIT,
    NEON_OP_CLASSGETSUPER,
    NEON_OP_CLASSINVOKESUPER,
    NEON_OP_CLASSINVOKESUPERSELF,
    NEON_OP_MAKERANGE,
    NEON_OP_MAKEARRAY,
    NEON_OP_MAKEDICT,
    NEON_OP_INDEXGET,
    NEON_OP_INDEXGETRANGED,
    NEON_OP_INDEXSET,
    NEON_OP_IMPORTIMPORT,
    NEON_OP_EXTRY,
    NEON_OP_EXPOPTRY,
    NEON_OP_EXPUBLISHTRY,
    NEON_OP_STRINGIFY,
    NEON_OP_SWITCH,
    NEON_OP_TYPEOF,
    NEON_OP_BREAK_PL
};

enum NNAstTokType
{
    NEON_ASTTOK_NEWLINE,
    NEON_ASTTOK_PARENOPEN,
    NEON_ASTTOK_PARENCLOSE,
    NEON_ASTTOK_BRACKETOPEN,
    NEON_ASTTOK_BRACKETCLOSE,
    NEON_ASTTOK_BRACEOPEN,
    NEON_ASTTOK_BRACECLOSE,
    NEON_ASTTOK_SEMICOLON,
    NEON_ASTTOK_COMMA,
    NEON_ASTTOK_BACKSLASH,
    NEON_ASTTOK_EXCLMARK,
    NEON_ASTTOK_NOTEQUAL,
    NEON_ASTTOK_COLON,
    NEON_ASTTOK_AT,
    NEON_ASTTOK_DOT,
    NEON_ASTTOK_DOUBLEDOT,
    NEON_ASTTOK_TRIPLEDOT,
    NEON_ASTTOK_PLUS,
    NEON_ASTTOK_PLUSASSIGN,
    NEON_ASTTOK_INCREMENT,
    NEON_ASTTOK_MINUS,
    NEON_ASTTOK_MINUSASSIGN,
    NEON_ASTTOK_DECREMENT,
    NEON_ASTTOK_MULTIPLY,
    NEON_ASTTOK_MULTASSIGN,
    NEON_ASTTOK_POWEROF,
    NEON_ASTTOK_POWASSIGN,
    NEON_ASTTOK_DIVIDE,
    NEON_ASTTOK_DIVASSIGN,
    NEON_ASTTOK_FLOOR,
    NEON_ASTTOK_ASSIGN,
    NEON_ASTTOK_EQUAL,
    NEON_ASTTOK_LESSTHAN,
    NEON_ASTTOK_LESSEQUAL,
    NEON_ASTTOK_LEFTSHIFT,
    NEON_ASTTOK_LEFTSHIFTASSIGN,
    NEON_ASTTOK_GREATERTHAN,
    NEON_ASTTOK_GREATER_EQ,
    NEON_ASTTOK_RIGHTSHIFT,
    NEON_ASTTOK_RIGHTSHIFTASSIGN,
    NEON_ASTTOK_MODULO,
    NEON_ASTTOK_PERCENT_EQ,
    NEON_ASTTOK_AMP,
    NEON_ASTTOK_AMP_EQ,
    NEON_ASTTOK_BAR,
    NEON_ASTTOK_BAR_EQ,
    NEON_ASTTOK_TILDE,
    NEON_ASTTOK_TILDE_EQ,
    NEON_ASTTOK_XOR,
    NEON_ASTTOK_XOR_EQ,
    NEON_ASTTOK_QUESTION,
    NEON_ASTTOK_KWAND,
    NEON_ASTTOK_KWAS,
    NEON_ASTTOK_KWASSERT,
    NEON_ASTTOK_KWBREAK,
    NEON_ASTTOK_KWCATCH,
    NEON_ASTTOK_KWCLASS,
    NEON_ASTTOK_KWCONTINUE,
    NEON_ASTTOK_KWFUNCTION,
    NEON_ASTTOK_KWDEFAULT,
    NEON_ASTTOK_KWTHROW,
    NEON_ASTTOK_KWDO,
    NEON_ASTTOK_KWECHO,
    NEON_ASTTOK_KWELSE,
    NEON_ASTTOK_KWFALSE,
    NEON_ASTTOK_KWFINALLY,
    NEON_ASTTOK_KWFOREACH,
    NEON_ASTTOK_KWIF,
    NEON_ASTTOK_KWIMPORT,
    NEON_ASTTOK_KWIN,
    NEON_ASTTOK_KWFOR,
    NEON_ASTTOK_KWNULL,
    NEON_ASTTOK_KWNEW,
    NEON_ASTTOK_KWOR,
    NEON_ASTTOK_KWSUPER,
    NEON_ASTTOK_KWRETURN,
    NEON_ASTTOK_KWTHIS,
    NEON_ASTTOK_KWSTATIC,
    NEON_ASTTOK_KWTRUE,
    NEON_ASTTOK_KWTRY,
    NEON_ASTTOK_KWTYPEOF,
    NEON_ASTTOK_KWSWITCH,
    NEON_ASTTOK_KWVAR,
    NEON_ASTTOK_KWCASE,
    NEON_ASTTOK_KWWHILE,
    NEON_ASTTOK_LITERAL,
    NEON_ASTTOK_LITNUMREG,
    NEON_ASTTOK_LITNUMBIN,
    NEON_ASTTOK_LITNUMOCT,
    NEON_ASTTOK_LITNUMHEX,
    NEON_ASTTOK_IDENTNORMAL,
    NEON_ASTTOK_DECORATOR,
    NEON_ASTTOK_INTERPOLATION,
    NEON_ASTTOK_EOF,
    NEON_ASTTOK_ERROR,
    NEON_ASTTOK_KWEMPTY,
    NEON_ASTTOK_UNDEFINED,
    NEON_ASTTOK_TOKCOUNT
};

enum NNAstPrecedence
{
    NEON_ASTPREC_NONE,

    /* =, &=, |=, *=, +=, -=, /=, **=, %=, >>=, <<=, ^=, //= */
    NEON_ASTPREC_ASSIGNMENT,
    /* ~= ?: */
    NEON_ASTPREC_CONDITIONAL,
    /* 'or' || */
    NEON_ASTPREC_OR,
    /* 'and' && */
    NEON_ASTPREC_AND,
    /* ==, != */
    NEON_ASTPREC_EQUALITY,
    /* <, >, <=, >= */
    NEON_ASTPREC_COMPARISON,
    /* | */
    NEON_ASTPREC_BITOR,
    /* ^ */
    NEON_ASTPREC_BITXOR,
    /* & */
    NEON_ASTPREC_BITAND,
    /* <<, >> */
    NEON_ASTPREC_SHIFT,
    /* .. */
    NEON_ASTPREC_RANGE,
    /* +, - */
    NEON_ASTPREC_TERM,
    /* *, /, %, **, // */
    NEON_ASTPREC_FACTOR,
    /* !, -, ~, (++, -- this two will now be treated as statements) */
    NEON_ASTPREC_UNARY,
    /* ., () */
    NEON_ASTPREC_CALL,
    NEON_ASTPREC_PRIMARY
};

enum NNFuncType
{
    NEON_FUNCTYPE_ANONYMOUS,
    NEON_FUNCTYPE_FUNCTION,
    NEON_FUNCTYPE_METHOD,
    NEON_FUNCTYPE_INITIALIZER,
    NEON_FUNCTYPE_PRIVATE,
    NEON_FUNCTYPE_STATIC,
    NEON_FUNCTYPE_SCRIPT
};

enum NNStatus
{
    NEON_STATUS_OK,
    NEON_STATUS_FAILCOMPILE,
    NEON_STATUS_FAILRUNTIME
};

enum NNPrMode
{
    NEON_PRMODE_UNDEFINED,
    NEON_PRMODE_STRING,
    NEON_PRMODE_FILE
};

enum NNObjType
{
    /* containers */
    NEON_OBJTYPE_STRING,
    NEON_OBJTYPE_RANGE,
    NEON_OBJTYPE_ARRAY,
    NEON_OBJTYPE_DICT,
    NEON_OBJTYPE_FILE,

    /* base object types */
    NEON_OBJTYPE_UPVALUE,
    NEON_OBJTYPE_FUNCBOUND,
    NEON_OBJTYPE_FUNCCLOSURE,
    NEON_OBJTYPE_FUNCSCRIPT,
    NEON_OBJTYPE_INSTANCE,
    NEON_OBJTYPE_FUNCNATIVE,
    NEON_OBJTYPE_CLASS,

    /* non-user objects */
    NEON_OBJTYPE_MODULE,
    NEON_OBJTYPE_SWITCH,
    /* object type that can hold any C pointer */
    NEON_OBJTYPE_USERDATA
};

enum NNValType
{
    NEON_VALTYPE_NULL,
    NEON_VALTYPE_BOOL,
    NEON_VALTYPE_NUMBER,
    NEON_VALTYPE_OBJ,
    NEON_VALTYPE_EMPTY
};

enum NNFieldType
{
    NEON_PROPTYPE_INVALID,
    NEON_PROPTYPE_VALUE,
    /*
    * indicates this field contains a function, a pseudo-getter (i.e., ".length")
    * which is called upon getting
    */
    NEON_PROPTYPE_FUNCTION
};


enum NNColor
{
    NEON_COLOR_RESET,
    NEON_COLOR_RED,
    NEON_COLOR_GREEN,    
    NEON_COLOR_YELLOW,
    NEON_COLOR_BLUE,
    NEON_COLOR_MAGENTA,
    NEON_COLOR_CYAN
};

enum NNAstCompContext
{
    NEON_COMPCONTEXT_NONE,
    NEON_COMPCONTEXT_CLASS,
    NEON_COMPCONTEXT_ARRAY,
    NEON_COMPCONTEXT_NESTEDFUNCTION
};

typedef enum /**/NNAstCompContext NNAstCompContext;
typedef enum /**/ NNColor NNColor;
typedef enum /**/NNFieldType NNFieldType;
typedef enum /**/ NNValType NNValType;
typedef enum /**/ NNOpCode NNOpCode;
typedef enum /**/ NNFuncType NNFuncType;
typedef enum /**/ NNObjType NNObjType;
typedef enum /**/ NNStatus NNStatus;
typedef enum /**/ NNAstTokType NNAstTokType;
typedef enum /**/ NNAstPrecedence NNAstPrecedence;
typedef enum /**/ NNPrMode NNPrMode;

typedef struct /**/NNFormatInfo NNFormatInfo;
typedef struct /**/NNIOResult NNIOResult;
typedef struct /**/ NNAstFuncCompiler NNAstFuncCompiler;
typedef struct /**/ NNObject NNObject;
typedef struct /**/ NNObjString NNObjString;
typedef struct /**/ NNObjArray NNObjArray;
typedef struct /**/ NNObjUpvalue NNObjUpvalue;
typedef struct /**/ NNObjClass NNObjClass;
typedef struct /**/ NNObjFuncNative NNObjFuncNative;
typedef struct /**/ NNObjModule NNObjModule;
typedef struct /**/ NNObjFuncScript NNObjFuncScript;
typedef struct /**/ NNObjFuncClosure NNObjFuncClosure;
typedef struct /**/ NNObjInstance NNObjInstance;
typedef struct /**/ NNObjFuncBound NNObjFuncBound;
typedef struct /**/ NNObjRange NNObjRange;
typedef struct /**/ NNObjDict NNObjDict;
typedef struct /**/ NNObjFile NNObjFile;
typedef struct /**/ NNObjSwitch NNObjSwitch;
typedef struct /**/ NNObjUserdata NNObjUserdata;
typedef union /**/ NNDoubleHashUnion NNDoubleHashUnion;
typedef struct /**/ NNValue NNValue;
typedef struct /**/NNPropGetSet NNPropGetSet;
typedef struct /**/NNProperty NNProperty;
typedef struct /**/ NNValArray NNValArray;
typedef struct /**/ NNBlob NNBlob;
typedef struct /**/ NNHashEntry NNHashEntry;
typedef struct /**/ NNHashTable NNHashTable;
typedef struct /**/ NNExceptionFrame NNExceptionFrame;
typedef struct /**/ NNCallFrame NNCallFrame;
typedef struct /**/ NNAstToken NNAstToken;
typedef struct /**/ NNAstLexer NNAstLexer;
typedef struct /**/ NNAstLocal NNAstLocal;
typedef struct /**/ NNAstUpvalue NNAstUpvalue;
typedef struct /**/ NNAstClassCompiler NNAstClassCompiler;
typedef struct /**/ NNAstParser NNAstParser;
typedef struct /**/ NNAstRule NNAstRule;
typedef struct /**/ NNRegFunc NNRegFunc;
typedef struct /**/ NNRegField NNRegField;
typedef struct /**/ NNRegClass NNRegClass;
typedef struct /**/ NNRegModule NNRegModule;
typedef struct /**/ NNState NNState;
typedef struct /**/ NNPrinter NNPrinter;
typedef struct /**/ NNArgCheck NNArgCheck;
typedef struct /**/ NNArguments NNArguments;
typedef struct /**/NNInstruction NNInstruction;
typedef struct utf8iterator_t utf8iterator_t;


typedef NNValue (*NNNativeFN)(NNState*, NNArguments*);
typedef void (*NNPtrFreeFN)(void*);
typedef bool (*NNAstParsePrefixFN)(NNAstParser*, bool);
typedef bool (*NNAstParseInfixFN)(NNAstParser*, NNAstToken, bool);
typedef NNValue (*NNClassFieldFN)(NNState*);
typedef void (*NNModLoaderFN)(NNState*);
typedef NNRegModule* (*NNModInitFN)(NNState*);
typedef double(*nnbinopfunc_t)(double, double);

struct utf8iterator_t
{
    /*input string pointer */
    const char* plainstr;

    /* input string length */
    uint32_t plainlen;

    /* the codepoint, or char */
    uint32_t codepoint;

    /* character size in bytes */
    uint8_t charsize;

    /* current character position */
    uint32_t currpos;

    /* next character position */
    uint32_t nextpos;

    /* number of counter characters currently */
    uint32_t currcount;
};

union NNDoubleHashUnion
{
    uint64_t bits;
    double num;
};

struct NNIOResult
{
    bool success;
    char* data;
    size_t length;    
};

struct NNPrinter
{
    /* if file: should be closed when writer is destroyed? */
    bool shouldclose;
    /* if file: should write operations be flushed via fflush()? */
    bool shouldflush;
    /* if string: true if $strbuf was taken via nn_printer_take */
    bool stringtaken;
    /* was this writer instance created on stack? */
    bool fromstack;
    bool shortenvalues;
    size_t maxvallength;
    /* the mode that determines what writer actually does */
    NNPrMode wrmode;
    NNState* pvm;
    StringBuffer* strbuf;
    FILE* handle;
};

struct NNFormatInfo
{
    /* length of the format string */
    size_t fmtlen;
    /* the actual format string */
    const char* fmtstr;
    /* destination writer */
    NNPrinter* writer;
    NNState* pvm;
};

struct NNValue
{
    NNValType type;
    union
    {
        bool vbool;
        double vfltnum;
        NNObject* vobjpointer;
    } valunion;
};

struct NNObject
{
    NNObjType type;
    bool mark;
    NNState* pvm;
    /*
    // when an object is marked as stale, it means that the
    // GC will never collect this object. This can be useful
    // for library/package objects that want to reuse native
    // objects in their types/pointers. The GC cannot reach
    // them yet, so it's best for them to be kept stale.
    */
    bool stale;
    NNObject* next;
};

struct NNPropGetSet
{
    NNValue getter;
    NNValue setter;
};

struct NNProperty
{
    NNFieldType type;
    NNValue value;
    bool havegetset;
    NNPropGetSet getset;
};

#if 0
struct NNValArray
{
    NNState* pvm;
    /* type size of the stored value (via sizeof) */
    int tsize;
    /* how many entries are currently stored? */
    int count;
    /* how many entries can be stored before growing? */
    int capacity;
    NNValue* values;
};
#endif

struct NNValArray
{
    NNState* pvm;
    size_t listcapacity;
    size_t listcount;
    const char* listname;
    NNValue* listitems;
};



struct NNInstruction
{
    /* is this instruction an opcode? */
    bool isop;
    /* opcode or value */
    uint8_t code;
    /* line corresponding to where this instruction was emitted */
    int srcline;
};

struct NNBlob
{
    int count;
    int capacity;
    NNInstruction* instrucs;
    NNValArray* constants;
    NNValArray* argdefvals;
};

struct NNHashEntry
{
    NNValue key;
    NNProperty value;
};

struct NNHashTable
{
    /*
    * FIXME: extremely stupid hack: $active ensures that a table that was destroyed
    * does not get marked again, et cetera.
    * since nn_table_destroy() zeroes the data before freeing, $active will be
    * false, and thus, no further marking occurs.
    * obviously the reason is that somewhere a table (from NNObjInstance) is being
    * read after being freed, but for now, this will work(-ish).
    */
    bool active;
    int count;
    int capacity;
    NNState* pvm;
    NNHashEntry* entries;
};

struct NNObjString
{
    NNObject objpadding;
    uint32_t hash;
    StringBuffer* sbuf;
};

struct NNObjUpvalue
{
    NNObject objpadding;
    int stackpos;
    NNValue closed;
    NNValue location;
    NNObjUpvalue* next;
};

struct NNObjModule
{
    NNObject objpadding;
    bool imported;
    NNHashTable* deftable;
    NNObjString* name;
    NNObjString* physicalpath;
    void* preloader;
    void* unloader;
    void* handle;
};

struct NNObjFuncScript
{
    NNObject objpadding;
    NNFuncType type;
    int arity;
    int upvalcount;
    bool isvariadic;
    NNBlob blob;
    NNObjString* name;
    NNObjModule* module;
};

struct NNObjFuncClosure
{
    NNObject objpadding;
    int upvalcount;
    NNObjFuncScript* scriptfunc;
    NNObjUpvalue** upvalues;
};

struct NNObjClass
{
    NNObject objpadding;

    /*
    * the constructor, if any. defaults to <empty>, and if not <empty>, expects to be
    * some callable value.
    */
    NNValue constructor;
    NNValue destructor;

    /*
    * when declaring a class, $instprops (their names, and initial values) are
    * copied to NNObjInstance::properties.
    * so `$instprops["something"] = somefunction;` remains untouched *until* an
    * instance of this class is created.
    */
    NNHashTable* instprops;

    /*
    * static, unchangeable(-ish) values. intended for values that are not unique, but shared
    * across classes, without being copied.
    */
    NNHashTable* staticproperties;

    /*
    * method table; they are currently not unique when instantiated; instead, they are
    * read from $methods as-is. this includes adding methods!
    * TODO: introduce a new hashtable field for NNObjInstance for unique methods, perhaps?
    * right now, this method just prevents unnecessary copying.
    */
    NNHashTable* methods;
    NNHashTable* staticmethods;
    NNObjString* name;
    NNObjClass* superclass;
};

struct NNObjInstance
{
    NNObject objpadding;
    /*
    * whether this instance is still "active", i.e., not destroyed, deallocated, etc.
    * in rare circumstances s
    */
    bool active;
    NNHashTable* properties;
    NNObjClass* klass;
};

struct NNObjFuncBound
{
    NNObject objpadding;
    NNValue receiver;
    NNObjFuncClosure* method;
};

struct NNObjFuncNative
{
    NNObject objpadding;
    NNFuncType type;
    const char* name;
    NNNativeFN natfunc;
    void* userptr;
};

struct NNObjArray
{
    NNObject objpadding;
    NNValArray* varray;
};

struct NNObjRange
{
    NNObject objpadding;
    int lower;
    int upper;
    int range;
};

struct NNObjDict
{
    NNObject objpadding;
    NNValArray* names;
    NNHashTable* htab;
};

struct NNObjFile
{
    NNObject objpadding;
    bool isopen;
    bool isstd;
    bool istty;
    int number;
    FILE* handle;
    NNObjString* mode;
    NNObjString* path;
};

struct NNObjSwitch
{
    NNObject objpadding;
    int defaultjump;
    int exitjump;
    NNHashTable* table;
};

struct NNObjUserdata
{
    NNObject objpadding;
    void* pointer;
    char* name;
    NNPtrFreeFN ondestroyfn;
};

struct NNExceptionFrame
{
    uint16_t address;
    uint16_t finallyaddress;
    NNObjClass* klass;
};

struct NNCallFrame
{
    int handlercount;
    int gcprotcount;
    int stackslotpos;
    NNInstruction* inscode;
    NNObjFuncClosure* closure;
    NNExceptionFrame handlers[NEON_CFG_MAXEXCEPTHANDLERS];
};

struct NNState
{
    struct
    {
        /* for switching through the command line args... */
        bool enablewarnings;
        bool dumpbytecode;
        bool exitafterbytecode;
        bool shoulddumpstack;
        bool enablestrictmode;
        bool showfullstack;
        bool enableapidebug;
        bool enableastdebug;
    } conf;

    struct
    {
        size_t stackidx;
        size_t stackcapacity;
        size_t framecapacity;
        size_t framecount;
        NNInstruction currentinstr;
        NNCallFrame* currentframe;
        NNObjUpvalue* openupvalues;
        NNObject* linkedobjects;
        NNCallFrame* framevalues;
        NNValue* stackvalues;
    } vmstate;

    struct
    {
        int graycount;
        int graycapacity;
        int bytesallocated;
        int nextgc;
        NNObject** graystack;
    } gcstate;


    struct {
        NNObjClass* stdexception;
        NNObjClass* syntaxerror;
        NNObjClass* asserterror;
        NNObjClass* ioerror;
        NNObjClass* oserror;
        NNObjClass* argumenterror;
    } exceptions;

    void* memuserptr;


    char* rootphysfile;

    NNObjDict* envdict;

    NNObjString* constructorname;
    NNObjModule* topmodule;
    NNValArray* importpath;

    /* objects tracker */
    NNHashTable* modules;
    NNHashTable* strings;
    NNHashTable* globals;

    /* object public methods */
    NNObjClass* classprimprocess;
    NNObjClass* classprimobject;
    NNObjClass* classprimnumber;
    NNObjClass* classprimstring;
    NNObjClass* classprimarray;
    NNObjClass* classprimdict;
    NNObjClass* classprimfile;
    NNObjClass* classprimrange;
    NNObjClass* classprimmath;

    bool isrepl;
    bool markvalue;
    NNObjArray* cliargv;
    NNObjString* clidirectory;
    NNObjFile* filestdout;
    NNObjFile* filestderr;
    NNObjFile* filestdin;

    /* miscellaneous */
    NNPrinter* stdoutprinter;
    NNPrinter* stderrprinter;
    NNPrinter* debugwriter;
};

struct NNAstToken
{
    bool isglobal;
    NNAstTokType type;
    const char* start;
    int length;
    int line;
};

struct NNAstLexer
{
    NNState* pvm;
    const char* start;
    const char* sourceptr;
    int line;
    int tplstringcount;
    int tplstringbuffer[NEON_CFG_ASTMAXSTRTPLDEPTH];
};

struct NNAstLocal
{
    bool iscaptured;
    int depth;
    NNAstToken name;
};

struct NNAstUpvalue
{
    bool islocal;
    uint16_t index;
};

struct NNAstFuncCompiler
{
    int localcount;
    int scopedepth;
    int handlercount;
    bool fromimport;
    NNAstFuncCompiler* enclosing;
    /* current function */
    NNObjFuncScript* targetfunc;
    NNFuncType type;
    NNAstLocal locals[NEON_CFG_ASTMAXLOCALS];
    NNAstUpvalue upvalues[NEON_CFG_ASTMAXUPVALS];
};

struct NNAstClassCompiler
{
    bool hassuperclass;
    NNAstClassCompiler* enclosing;
    NNAstToken name;
};

struct NNAstParser
{
    bool haderror;
    bool panicmode;
    bool isreturning;
    bool istrying;
    bool replcanecho;
    bool keeplastvalue;
    bool lastwasstatement;
    bool infunction;
    /* used for tracking loops for the continue statement... */
    int innermostloopstart;
    int innermostloopscopedepth;
    int blockcount;
    /* the context in which the parser resides; none (outer level), inside a class, dict, array, etc */
    NNAstCompContext compcontext;
    const char* currentfile;
    NNState* pvm;
    NNAstLexer* lexer;
    NNAstToken currtoken;
    NNAstToken prevtoken;
    NNAstFuncCompiler* currentfunccompiler;
    NNAstClassCompiler* currentclasscompiler;
    NNObjModule* currentmodule;
};

struct NNAstRule
{
    NNAstParsePrefixFN prefix;
    NNAstParseInfixFN infix;
    NNAstPrecedence precedence;
};

struct NNRegFunc
{
    const char* name;
    bool isstatic;
    NNNativeFN function;
};

struct NNRegField
{
    const char* name;
    bool isstatic;
    NNClassFieldFN fieldvalfn;
};

struct NNRegClass
{
    const char* name;
    NNRegField* fields;
    NNRegFunc* functions;
};

struct NNRegModule
{
    /*
    * the name of this module.
    * note: if the name must be preserved, copy it; it is only a pointer to a
    * string that gets freed past loading.
    */
    const char* name;

    /* exported fields, if any. */
    NNRegField* fields;

    /* regular functions, if any. */
    NNRegFunc* functions;

    /* exported classes, if any.
    * i.e.:
    * {"Stuff",
    *   (NNRegField[]){
    *       {"enabled", true},
    *       ...
    *   },
    *   (NNRegFunc[]){
    *       {"isStuff", myclass_fn_isstuff},
    *       ...
    * }})*/
    NNRegClass* classes;

    /* function that is called directly upon loading the module. can be NULL. */
    NNModLoaderFN preloader;

    /* function that is called before unloading the module. can be NULL. */
    NNModLoaderFN unloader;
};

struct NNArgCheck
{
    NNState* pvm;
    const char* name;
    int argc;
    NNValue* argv;
};

struct NNArguments
{
    /* number of arguments */
    size_t count;

    /* the actual arguments */
    NNValue* args;

    /*
    * if called within something with an instance, this is where its reference will be.
    * otherwise, empty.
    */
    NNValue thisval;

    /*
    * the name of the function being called.
    * note: this is the *declarative* name; meaning, on an alias'd func, it will be
    * the name of the origin.
    */
    const char* name;

    /*
    * a userpointer, if declared with a userpointer. otherwise NULL.
    */
    void* userptr;
};

#include "prot.inc"

/*
via: https://github.com/adrianwk94/utf8-iterator
UTF-8 Iterator. Version 0.1.3

Original code by Adrian Guerrero Vera (adrianwk94@gmail.com)
MIT License
Copyright (c) 2016 Adrian Guerrero Vera

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

neon::Value nn_util_stringutf8chars(neon::State* state, neon::Arguments* args, bool onlycodepoint)
{
    int cp;
    const char* cstr;
    utf8iterator_t iter;
    neon::Array* res;
    neon::String* os;
    neon::String* instr;
    (void)state;
    instr = args->thisval.asString();
    res = neon::Array::make(state);
    nn_utf8iter_init(&iter, instr->data(), instr->length());
    while (nn_utf8iter_next(&iter))
    {
        cp = iter.codepoint;
        cstr = nn_utf8iter_getchar(&iter);
        if(onlycodepoint)
        {
            res->push(neon::Value::makeNumber(cp));
        }
        else
        {
            os = neon::String::copy(state, cstr, iter.charsize);
            res->push(neon::Value::fromObject(os));
        }
    }
    return neon::Value::fromObject(res);
}

*/

/* allows you to set a custom length. */
NEON_INLINE void nn_utf8iter_init(utf8iterator_t* iter, const char* ptr, uint32_t length)
{
    iter->plainstr = ptr;
    iter->plainlen = length;
    iter->codepoint = 0;
    iter->currpos = 0;
    iter->nextpos = 0;
    iter->currcount = 0;
}

/* calculate the number of bytes a UTF8 character occupies in a string. */
NEON_INLINE uint8_t nn_utf8iter_charsize(const char* character)
{
    if(character == NULL)
    {
        return 0;
    }
    if(character[0] == 0)
    {
        return 0;
    }
    if((character[0] & 0x80) == 0)
    {
        return 1;
    }
    else if((character[0] & 0xE0) == 0xC0)
    {
        return 2;
    }
    else if((character[0] & 0xF0) == 0xE0)
    {
        return 3;
    }
    else if((character[0] & 0xF8) == 0xF0)
    {
        return 4;
    }
    else if((character[0] & 0xFC) == 0xF8)
    {
        return 5;
    }
    else if((character[0] & 0xFE) == 0xFC)
    {
        return 6;
    }
    return 0;
}

NEON_INLINE uint32_t nn_utf8iter_converter(const char* character, uint8_t size)
{
    uint8_t i;
    static uint32_t codepoint = 0;
    static const uint8_t g_utf8iter_table_unicode[] = { 0, 0, 0x1F, 0xF, 0x7, 0x3, 0x1 };
    if(size == 0)
    {
        return 0;
    }
    if(character == NULL)
    {
        return 0;
    }
    if(character[0] == 0)
    {
        return 0;
    }
    if(size == 1)
    {
        return character[0];
    }
    codepoint = g_utf8iter_table_unicode[size] & character[0];
    for(i = 1; i < size; i++)
    {
        codepoint = codepoint << 6;
        codepoint = codepoint | (character[i] & 0x3F);
    }
    return codepoint;
}

/* returns 1 if there is a character in the next position. If there is not, return 0. */
NEON_INLINE uint8_t nn_utf8iter_next(utf8iterator_t* iter)
{
    const char* pointer;
    if(iter == NULL)
    {
        return 0;
    }
    if(iter->plainstr == NULL)
    {
        return 0;
    }
    if(iter->nextpos < iter->plainlen)
    {
        iter->currpos = iter->nextpos;
        /* Set Current Pointer */
        pointer = iter->plainstr + iter->nextpos;
        iter->charsize = nn_utf8iter_charsize(pointer);
        if(iter->charsize == 0)
        {
            return 0;
        }
        iter->nextpos = iter->nextpos + iter->charsize;
        iter->codepoint = nn_utf8iter_converter(pointer, iter->charsize);
        if(iter->codepoint == 0)
        {
            return 0;
        }
        iter->currcount++;
        return 1;
    }
    iter->currpos = iter->nextpos;
    return 0;
}

/* return current character in UFT8 - no same that iter.codepoint (not codepoint/unicode) */
NEON_INLINE const char* nn_utf8iter_getchar(utf8iterator_t* iter)
{
    uint8_t i;
    const char* pointer;
    static char str[10];
    str[0] = '\0';
    if(iter == NULL)
    {
        return str;
    }
    if(iter->plainstr == NULL)
    {
        return str;
    }
    if(iter->charsize == 0)
    {
        return str;
    }
    if(iter->charsize == 1)
    {
        str[0] = iter->plainstr[iter->currpos];
        str[1] = '\0';
        return str;
    }
    pointer = iter->plainstr + iter->currpos;
    for(i = 0; i < iter->charsize; i++)
    {
        str[i] = pointer[i];
    }
    str[iter->charsize] = '\0';
    return str;
}

NEON_FORCEINLINE bool nn_value_isobject(NNValue v)
{
    return ((v).type == NEON_VALTYPE_OBJ);
}

NEON_FORCEINLINE NNObject* nn_value_asobject(NNValue v)
{
    return ((v).valunion.vobjpointer);
}

NEON_FORCEINLINE bool nn_value_isobjtype(NNValue v, NNObjType t)
{
    return nn_value_isobject(v) && nn_value_asobject(v)->type == t;
}

NEON_FORCEINLINE bool nn_value_isnull(NNValue v)
{
    return ((v).type == NEON_VALTYPE_NULL);
}

NEON_FORCEINLINE bool nn_value_isbool(NNValue v)
{
    return ((v).type == NEON_VALTYPE_BOOL);
}

NEON_FORCEINLINE bool nn_value_isnumber(NNValue v)
{
    return ((v).type == NEON_VALTYPE_NUMBER);
}

NEON_FORCEINLINE bool nn_value_isempty(NNValue v)
{
    return ((v).type == NEON_VALTYPE_EMPTY);
}

NEON_FORCEINLINE bool nn_value_isstring(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_STRING);
}

NEON_FORCEINLINE bool nn_value_isfuncnative(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_FUNCNATIVE);
}

NEON_FORCEINLINE bool nn_value_isfuncscript(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_FUNCSCRIPT);
}

NEON_FORCEINLINE bool nn_value_isfuncclosure(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_FUNCCLOSURE);
}

NEON_FORCEINLINE bool nn_value_isfuncbound(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_FUNCBOUND);
}

NEON_FORCEINLINE bool nn_value_isclass(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_CLASS);
}

NEON_FORCEINLINE bool nn_value_isinstance(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_INSTANCE);
}

NEON_FORCEINLINE bool nn_value_isarray(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_ARRAY);
}

NEON_FORCEINLINE bool nn_value_isdict(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_DICT);
}

NEON_FORCEINLINE bool nn_value_isfile(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_FILE);
}

NEON_FORCEINLINE bool nn_value_isrange(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_RANGE);
}

NEON_FORCEINLINE bool nn_value_ismodule(NNValue v)
{
    return nn_value_isobjtype(v, NEON_OBJTYPE_MODULE);
}

NEON_FORCEINLINE bool nn_value_iscallable(NNValue v)
{
    return (
        nn_value_isclass(v) ||
        nn_value_isfuncscript(v) ||
        nn_value_isfuncclosure(v) ||
        nn_value_isfuncbound(v) ||
        nn_value_isfuncnative(v)
    );
}

NEON_FORCEINLINE NNObjType nn_value_objtype(NNValue v)
{
    return nn_value_asobject(v)->type;
}

NEON_FORCEINLINE bool nn_value_asbool(NNValue v)
{
    return ((v).valunion.vbool);
}

NEON_FORCEINLINE double nn_value_asnumber(NNValue v)
{
    return ((v).valunion.vfltnum);
}

NEON_FORCEINLINE NNObjString* nn_value_asstring(NNValue v)
{
    return ((NNObjString*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjFuncNative* nn_value_asfuncnative(NNValue v)
{
    return ((NNObjFuncNative*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjFuncScript* nn_value_asfuncscript(NNValue v)
{
    return ((NNObjFuncScript*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjFuncClosure* nn_value_asfuncclosure(NNValue v)
{
    return ((NNObjFuncClosure*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjClass* nn_value_asclass(NNValue v)
{
    return ((NNObjClass*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjInstance* nn_value_asinstance(NNValue v)
{
    return ((NNObjInstance*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjFuncBound* nn_value_asfuncbound(NNValue v)
{
    return ((NNObjFuncBound*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjSwitch* nn_value_asswitch(NNValue v)
{
    return ((NNObjSwitch*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjUserdata* nn_value_asuserdata(NNValue v)
{
    return ((NNObjUserdata*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjModule* nn_value_asmodule(NNValue v)
{
    return ((NNObjModule*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjArray* nn_value_asarray(NNValue v)
{
    return ((NNObjArray*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjDict* nn_value_asdict(NNValue v)
{
    return ((NNObjDict*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjFile* nn_value_asfile(NNValue v)
{
    return ((NNObjFile*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNObjRange* nn_value_asrange(NNValue v)
{
    return ((NNObjRange*)nn_value_asobject(v));
}

NEON_FORCEINLINE NNValue nn_value_makevalue(NNValType type)
{
    NNValue v;
    v.type = type;
    return v;
}

NEON_FORCEINLINE NNValue nn_value_makeempty()
{
    return nn_value_makevalue(NEON_VALTYPE_EMPTY);
}

NEON_FORCEINLINE NNValue nn_value_makenull()
{
    NNValue v;
    v = nn_value_makevalue(NEON_VALTYPE_NULL);
    return v;
}

NEON_FORCEINLINE NNValue nn_value_makebool(bool b)
{
    NNValue v;
    v = nn_value_makevalue(NEON_VALTYPE_BOOL);
    v.valunion.vbool = b;
    return v;
}

NEON_FORCEINLINE NNValue nn_value_makenumber(double d)
{
    NNValue v;
    v = nn_value_makevalue(NEON_VALTYPE_NUMBER);
    v.valunion.vfltnum = d;
    return v;
}

NEON_FORCEINLINE NNValue nn_value_makeint(int i)
{
    NNValue v;
    v = nn_value_makevalue(NEON_VALTYPE_NUMBER);
    v.valunion.vfltnum = i;
    return v;
}

#define nn_value_fromobject(obj) nn_value_fromobject_actual((NNObject*)obj)

NEON_FORCEINLINE NNValue nn_value_fromobject_actual(NNObject* obj)
{
    NNValue v;
    v = nn_value_makevalue(NEON_VALTYPE_OBJ);
    v.valunion.vobjpointer = obj;
    return v;
}

void* nn_util_rawrealloc(void* userptr, void* ptr, size_t size)
{
    (void)userptr;
    return realloc(ptr, size);
}

void* nn_util_rawmalloc(void* userptr, size_t size)
{
    (void)userptr;
    return malloc(size);
}

void* nn_util_rawcalloc(void* userptr, size_t count, size_t size)
{
    (void)userptr;
    return calloc(count, size);
}

void nn_util_rawfree(void* userptr, void* ptr)
{
    (void)userptr;
    free(ptr);
}

void* nn_util_memrealloc(NNState* state, void* ptr, size_t size)
{
    return nn_util_rawrealloc(state->memuserptr, ptr, size);
}

void* nn_util_memmalloc(NNState* state, size_t size)
{
    return nn_util_rawmalloc(state->memuserptr, size);
}

void* nn_util_memcalloc(NNState* state, size_t count, size_t size)
{
    return nn_util_rawcalloc(state->memuserptr, count, size);
}

void nn_util_memfree(NNState* state, void* ptr)
{
    nn_util_rawfree(state->memuserptr, ptr);
}

#include "vallist.h"

NNObject* nn_gcmem_protect(NNState* state, NNObject* object)
{
    size_t frpos;
    nn_vm_stackpush(state, nn_value_fromobject(object));
    frpos = 0;
    if(state->vmstate.framecount > 0)
    {
        frpos = state->vmstate.framecount - 1;
    }
    state->vmstate.framevalues[frpos].gcprotcount++;
    return object;
}

void nn_gcmem_clearprotect(NNState* state)
{
    size_t frpos;
    NNCallFrame* frame;
    frpos = 0;
    if(state->vmstate.framecount > 0)
    {
        frpos = state->vmstate.framecount - 1;
    }
    frame = &state->vmstate.framevalues[frpos];
    if(frame->gcprotcount > 0)
    {
        state->vmstate.stackidx -= frame->gcprotcount;
    }
    frame->gcprotcount = 0;
}

const char* nn_util_color(NNColor tc)
{
    #if !defined(NEON_CFG_FORCEDISABLECOLOR)
        bool istty;
        int fdstdout;
        int fdstderr;
        fdstdout = fileno(stdout);
        fdstderr = fileno(stderr);
        istty = (osfn_isatty(fdstderr) && osfn_isatty(fdstdout));
        if(istty)
        {
            switch(tc)
            {
                case NEON_COLOR_RESET:
                    return "\x1B[0m";
                case NEON_COLOR_RED:
                    return "\x1B[31m";
                case NEON_COLOR_GREEN:
                    return "\x1B[32m";
                case NEON_COLOR_YELLOW:
                    return "\x1B[33m";
                case NEON_COLOR_BLUE:
                    return "\x1B[34m";
                case NEON_COLOR_MAGENTA:
                    return "\x1B[35m";
                case NEON_COLOR_CYAN:
                    return "\x1B[36m";
            }
        }
    #else
        (void)tc;
    #endif
    return "";
}

char* nn_util_strndup(NNState* state, const char* src, size_t len)
{
    char* buf;
    buf = (char*)nn_util_memmalloc(state, sizeof(char) * (len+1));
    if(buf == NULL)
    {
        return NULL;
    }
    memset(buf, 0, len+1);
    memcpy(buf, src, len);
    return buf;
}

char* nn_util_strdup(NNState* state, const char* src)
{
    return nn_util_strndup(state, src, strlen(src));
}

char* nn_util_readhandle(NNState* state, FILE* hnd, size_t* dlen)
{
    long rawtold;
    /*
    * the value returned by ftell() may not necessarily be the same as
    * the amount that can be read.
    * since we only ever read a maximum of $toldlen, there will
    * be no memory trashing.
    */
    size_t toldlen;
    size_t actuallen;
    char* buf;
    if(fseek(hnd, 0, SEEK_END) == -1)
    {
        return NULL;
    }
    if((rawtold = ftell(hnd)) == -1)
    {
        return NULL;
    }
    toldlen = rawtold;
    if(fseek(hnd, 0, SEEK_SET) == -1)
    {
        return NULL;
    }
    buf = (char*)nn_gcmem_allocate(state, sizeof(char), toldlen + 1);
    memset(buf, 0, toldlen+1);
    if(buf != NULL)
    {
        actuallen = fread(buf, sizeof(char), toldlen, hnd);
        /*
        // optionally, read remainder:
        size_t tmplen;
        if(actuallen < toldlen)
        {
            tmplen = actuallen;
            actuallen += fread(buf+tmplen, sizeof(char), actuallen-toldlen, hnd);
            ...
        }
        // unlikely to be necessary, so not implemented.
        */
        if(dlen != NULL)
        {
            *dlen = actuallen;
        }
        return buf;
    }
    return NULL;
}

char* nn_util_readfile(NNState* state, const char* filename, size_t* dlen)
{
    char* b;
    FILE* fh;
    fh = fopen(filename, "rb");
    if(fh == NULL)
    {
        return NULL;
    }
    #if defined(NEON_PLAT_ISWINDOWS)
        _setmode(fileno(fh), _O_BINARY);
    #endif
    b = nn_util_readhandle(state, fh, dlen);
    fclose(fh);
    return b;
}

/* returns the number of bytes contained in a unicode character */
int nn_util_utf8numbytes(int value)
{
    if(value < 0)
    {
        return -1;
    }
    if(value <= 0x7f)
    {
        return 1;
    }
    if(value <= 0x7ff)
    {
        return 2;
    }
    if(value <= 0xffff)
    {
        return 3;
    }
    if(value <= 0x10ffff)
    {
        return 4;
    }
    return 0;
}

char* nn_util_utf8encode(NNState* state, unsigned int code, size_t* dlen)
{
    int count;
    char* chars;
    *dlen = 0;
    count = nn_util_utf8numbytes((int)code);
    if(count > 0)
    {
        *dlen = count;
        chars = (char*)nn_gcmem_allocate(state, sizeof(char), (size_t)count + 1);
        if(chars != NULL)
        {
            if(code <= 0x7F)
            {
                chars[0] = (char)(code & 0x7F);
                chars[1] = '\0';
            }
            else if(code <= 0x7FF)
            {
                /* one continuation byte */
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xC0 | (code & 0x1F));
            }
            else if(code <= 0xFFFF)
            {
                /* two continuation bytes */
                chars[2] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xE0 | (code & 0xF));
            }
            else if(code <= 0x10FFFF)
            {
                /* three continuation bytes */
                chars[3] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[2] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xF0 | (code & 0x7));
            }
            else
            {
                /* unicode replacement character */
                chars[2] = (char)0xEF;
                chars[1] = (char)0xBF;
                chars[0] = (char)0xBD;
            }
            return chars;
        }
    }
    return NULL;
}

int nn_util_utf8decode(const uint8_t* bytes, uint32_t length)
{
    int value;
    uint32_t remainingbytes;
    /* Single byte (i.e. fits in ASCII). */
    if(*bytes <= 0x7f)
    {
        return *bytes;
    }
    if((*bytes & 0xe0) == 0xc0)
    {
        /* Two byte sequence: 110xxxxx 10xxxxxx. */
        value = *bytes & 0x1f;
        remainingbytes = 1;
    }
    else if((*bytes & 0xf0) == 0xe0)
    {
        /* Three byte sequence: 1110xxxx	 10xxxxxx 10xxxxxx. */
        value = *bytes & 0x0f;
        remainingbytes = 2;
    }
    else if((*bytes & 0xf8) == 0xf0)
    {
        /* Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx. */
        value = *bytes & 0x07;
        remainingbytes = 3;
    }
    else
    {
        /* Invalid UTF-8 sequence. */
        return -1;
    }
    /* Don't read past the end of the buffer on truncated UTF-8. */
    if(remainingbytes > length - 1)
    {
        return -1;
    }
    while(remainingbytes > 0)
    {
        bytes++;
        remainingbytes--;
        /* Remaining bytes must be of form 10xxxxxx. */
        if((*bytes & 0xc0) != 0x80)
        {
            return -1;
        }
        value = value << 6 | (*bytes & 0x3f);
    }
    return value;
}

char* nn_util_utf8codepoint(const char* str, char* outcodepoint)
{
    if(0xf0 == (0xf8 & str[0]))
    {
        /* 4 byte utf8 codepoint */
        *outcodepoint = (
            ((0x07 & str[0]) << 18) |
            ((0x3f & str[1]) << 12) |
            ((0x3f & str[2]) << 6) |
            (0x3f & str[3])
        );
        str += 4;
    }
    else if(0xe0 == (0xf0 & str[0]))
    {
        /* 3 byte utf8 codepoint */
        *outcodepoint = (
            ((0x0f & str[0]) << 12) |
            ((0x3f & str[1]) << 6) |
            (0x3f & str[2])
        );
        str += 3;
    }
    else if(0xc0 == (0xe0 & str[0]))
    {
        /* 2 byte utf8 codepoint */
        *outcodepoint = (
            ((0x1f & str[0]) << 6) |
            (0x3f & str[1])
        );
        str += 2;
    }
    else
    {
        /* 1 byte utf8 codepoint otherwise */
        *outcodepoint = str[0];
        str += 1;
    }
    return (char*)str;
}

char* nn_util_utf8strstr(const char* haystack, const char* needle)
{
    char throwawaycodepoint;
    const char* n;
    const char* maybematch;
    throwawaycodepoint = 0;
    /* if needle has no utf8 codepoints before the null terminating
     * byte then return haystack */
    if('\0' == *needle)
    {
        return (char*)haystack;
    }
    while('\0' != *haystack)
    {
        maybematch = haystack;
        n = needle;
        while(*haystack == *n && (*haystack != '\0' && *n != '\0'))
        {
            n++;
            haystack++;
        }
        if('\0' == *n)
        {
            /* we found the whole utf8 string for needle in haystack at
             * maybematch, so return it */
            return (char*)maybematch;
        }
        else
        {
            /* h could be in the middle of an unmatching utf8 codepoint,
             * so we need to march it on to the next character beginning
             * starting from the current character */
            haystack = nn_util_utf8codepoint(maybematch, &throwawaycodepoint);
        }
    }
    /* no match */
    return NULL;
}

/*
// returns a pointer to the beginning of the pos'th utf8 codepoint
// in the buffer at s
*/
char* nn_util_utf8index(char* s, int pos)
{
    ++pos;
    for(; *s; ++s)
    {
        if((*s & 0xC0) != 0x80)
        {
            --pos;
        }
        if(pos == 0)
        {
            return s;
        }
    }
    return NULL;
}

/*
// converts codepoint indexes start and end to byte offsets in the buffer at s
*/
void nn_util_utf8slice(char* s, int* start, int* end)
{
    char* p;
    p = nn_util_utf8index(s, *start);
    if(p != NULL)
    {
        *start = (int)(p - s);
    }
    else
    {
        *start = -1;
    }
    p = nn_util_utf8index(s, *end);
    if(p != NULL)
    {
        *end = (int)(p - s);
    }
    else
    {
        *end = (int)strlen(s);
    }
}

char* nn_util_strtoupper(char* str, size_t length)
{
    int c;
    size_t i;
    for(i=0; i<length; i++)
    {
        c = str[i];
        str[i] = toupper(c);
    }
    return str;
}

char* nn_util_strtolower(char* str, size_t length)
{
    int c;
    size_t i;
    for(i=0; i<length; i++)
    {
        c = str[i];
        str[i] = toupper(c);
    }
    return str;
}


#if 0
    #define NEON_APIDEBUG(state, ...) \
        if((NEON_UNLIKELY((state)->conf.enableapidebug))) \
        { \
            nn_state_apidebug(state, __FUNCTION__, __VA_ARGS__); \
        }
#else
    #define NEON_APIDEBUG(state, ...)
#endif

#if defined(__STRICT_ANSI__)
    #define NEON_ASTDEBUG(state, ...)
#else
    #define NEON_ASTDEBUG(state, ...) \
        if((NEON_UNLIKELY((state)->conf.enableastdebug))) \
        { \
            nn_state_astdebug(state, __FUNCTION__, __VA_ARGS__); \
        }
#endif

NEON_FORCEINLINE void nn_state_apidebugv(NNState* state, const char* funcname, const char* format, va_list va)
{
    (void)state;
    fprintf(stderr, "API CALL: to '%s': ", funcname);
    vfprintf(stderr, format, va);
    fprintf(stderr, "\n");
}

NEON_FORCEINLINE void nn_state_apidebug(NNState* state, const char* funcname, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    nn_state_apidebugv(state, funcname, format, va);
    va_end(va);
}

NEON_FORCEINLINE void nn_state_astdebugv(NNState* state, const char* funcname, const char* format, va_list va)
{
    (void)state;
    fprintf(stderr, "AST CALL: to '%s': ", funcname);
    vfprintf(stderr, format, va);
    fprintf(stderr, "\n");
}

NEON_INLINE void nn_state_astdebug(NNState* state, const char* funcname, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    nn_state_astdebugv(state, funcname, format, va);
    va_end(va);
}

void nn_gcmem_maybecollect(NNState* state, int addsize, bool wasnew)
{
    state->gcstate.bytesallocated += addsize;
    if(state->gcstate.nextgc > 0)
    {
        if(wasnew && state->gcstate.bytesallocated > state->gcstate.nextgc)
        {
            if(state->vmstate.currentframe && state->vmstate.currentframe->gcprotcount == 0)
            {
                nn_gcmem_collectgarbage(state);
            }
        }
    }
}

void* nn_gcmem_reallocate(NNState* state, void* pointer, size_t oldsize, size_t newsize)
{
    void* result;
    nn_gcmem_maybecollect(state, newsize - oldsize, newsize > oldsize);
    result = nn_util_memrealloc(state, pointer, newsize);
    /*
    // just in case reallocation fails... computers ain't infinite!
    */
    if(result == NULL)
    {
        fprintf(stderr, "fatal error: failed to allocate %ld bytes\n", newsize);
        abort();
    }
    return result;
}

void nn_gcmem_release(NNState* state, void* pointer, size_t oldsize)
{
    nn_gcmem_maybecollect(state, -oldsize, false);
    if(oldsize > 0)
    {
        memset(pointer, 0, oldsize);
    }
    nn_util_memfree(state, pointer);
    pointer = NULL;
}

void* nn_gcmem_allocate(NNState* state, size_t size, size_t amount)
{
    return nn_gcmem_reallocate(state, NULL, 0, size * amount);
}

void nn_gcmem_markobject(NNState* state, NNObject* object)
{
    if(object == NULL)
    {
        return;
    }
    if(object->mark == state->markvalue)
    {
        return;
    }
    #if defined(DEBUG_GC) && DEBUG_GC
    nn_printer_writefmt(state->debugwriter, "GC: marking object at <%p> ", (void*)object);
    nn_printer_printvalue(state->debugwriter, nn_value_fromobject(object), false);
    nn_printer_writefmt(state->debugwriter, "\n");
    #endif
    object->mark = state->markvalue;
    if(state->gcstate.graycapacity < state->gcstate.graycount + 1)
    {
        state->gcstate.graycapacity = GROW_CAPACITY(state->gcstate.graycapacity);
        state->gcstate.graystack = (NNObject**)nn_util_memrealloc(state, state->gcstate.graystack, sizeof(NNObject*) * state->gcstate.graycapacity);
        if(state->gcstate.graystack == NULL)
        {
            fflush(stdout);
            fprintf(stderr, "GC encountered an error");
            abort();
        }
    }
    state->gcstate.graystack[state->gcstate.graycount++] = object;
}

void nn_gcmem_markvalue(NNState* state, NNValue value)
{
    if(nn_value_isobject(value))
    {
        nn_gcmem_markobject(state, nn_value_asobject(value));
    }
}

void nn_gcmem_blackenobject(NNState* state, NNObject* object)
{
    #if defined(DEBUG_GC) && DEBUG_GC
    nn_printer_writefmt(state->debugwriter, "GC: blacken object at <%p> ", (void*)object);
    nn_printer_printvalue(state->debugwriter, nn_value_fromobject(object), false);
    nn_printer_writefmt(state->debugwriter, "\n");
    #endif
    switch(object->type)
    {
        case NEON_OBJTYPE_MODULE:
            {
                NNObjModule* module;
                module = (NNObjModule*)object;
                nn_table_mark(state, module->deftable);
            }
            break;
        case NEON_OBJTYPE_SWITCH:
            {
                NNObjSwitch* sw;
                sw = (NNObjSwitch*)object;
                nn_table_mark(state, sw->table);
            }
            break;
        case NEON_OBJTYPE_FILE:
            {
                NNObjFile* file;
                file = (NNObjFile*)object;
                nn_file_mark(state, file);
            }
            break;
        case NEON_OBJTYPE_DICT:
            {
                NNObjDict* dict;
                dict = (NNObjDict*)object;
                nn_vallist_mark(dict->names);
                nn_table_mark(state, dict->htab);
            }
            break;
        case NEON_OBJTYPE_ARRAY:
            {
                NNObjArray* list;
                list = (NNObjArray*)object;
                nn_vallist_mark(list->varray);
            }
            break;
        case NEON_OBJTYPE_FUNCBOUND:
            {
                NNObjFuncBound* bound;
                bound = (NNObjFuncBound*)object;
                nn_gcmem_markvalue(state, bound->receiver);
                nn_gcmem_markobject(state, (NNObject*)bound->method);
            }
            break;
        case NEON_OBJTYPE_CLASS:
            {
                NNObjClass* klass;
                klass = (NNObjClass*)object;
                nn_gcmem_markobject(state, (NNObject*)klass->name);
                nn_table_mark(state, klass->methods);
                nn_table_mark(state, klass->staticmethods);
                nn_table_mark(state, klass->staticproperties);
                nn_gcmem_markvalue(state, klass->constructor);
                nn_gcmem_markvalue(state, klass->destructor);
                if(klass->superclass != NULL)
                {
                    nn_gcmem_markobject(state, (NNObject*)klass->superclass);
                }
            }
            break;
        case NEON_OBJTYPE_FUNCCLOSURE:
            {
                int i;
                NNObjFuncClosure* closure;
                closure = (NNObjFuncClosure*)object;
                nn_gcmem_markobject(state, (NNObject*)closure->scriptfunc);
                for(i = 0; i < closure->upvalcount; i++)
                {
                    nn_gcmem_markobject(state, (NNObject*)closure->upvalues[i]);
                }
            }
            break;
        case NEON_OBJTYPE_FUNCSCRIPT:
            {
                NNObjFuncScript* function;
                function = (NNObjFuncScript*)object;
                nn_gcmem_markobject(state, (NNObject*)function->name);
                nn_gcmem_markobject(state, (NNObject*)function->module);
                nn_vallist_mark(function->blob.constants);
            }
            break;
        case NEON_OBJTYPE_INSTANCE:
            {
                NNObjInstance* instance;
                instance = (NNObjInstance*)object;
                nn_instance_mark(state, instance);
            }
            break;
        case NEON_OBJTYPE_UPVALUE:
            {
                nn_gcmem_markvalue(state, ((NNObjUpvalue*)object)->closed);
            }
            break;
        case NEON_OBJTYPE_RANGE:
        case NEON_OBJTYPE_FUNCNATIVE:
        case NEON_OBJTYPE_USERDATA:
        case NEON_OBJTYPE_STRING:
            break;
    }
}

void nn_gcmem_destroyobject(NNState* state, NNObject* object)
{
    #if defined(DEBUG_GC) && DEBUG_GC
    nn_printer_writefmt(state->debugwriter, "GC: freeing at <%p> of type %d\n", (void*)object, object->type);
    #endif
    if(object->stale)
    {
        return;
    }
    switch(object->type)
    {
        case NEON_OBJTYPE_MODULE:
            {
                NNObjModule* module;
                module = (NNObjModule*)object;
                nn_module_destroy(state, module);
                nn_gcmem_release(state, object, sizeof(NNObjModule));
            }
            break;
        case NEON_OBJTYPE_FILE:
            {
                NNObjFile* file;
                file = (NNObjFile*)object;
                nn_file_destroy(state, file);
            }
            break;
        case NEON_OBJTYPE_DICT:
            {
                NNObjDict* dict;
                dict = (NNObjDict*)object;
                nn_vallist_destroy(dict->names);
                nn_table_destroy(dict->htab);
                nn_gcmem_release(state, object, sizeof(NNObjDict));
            }
            break;
        case NEON_OBJTYPE_ARRAY:
            {
                NNObjArray* list;
                list = (NNObjArray*)object;
                nn_vallist_destroy(list->varray);
                nn_gcmem_release(state, object, sizeof(NNObjArray));
            }
            break;
        case NEON_OBJTYPE_FUNCBOUND:
            {
                /*
                // a closure may be bound to multiple instances
                // for this reason, we do not free closures when freeing bound methods
                */
                nn_gcmem_release(state, object, sizeof(NNObjFuncBound));
            }
            break;
        case NEON_OBJTYPE_CLASS:
            {
                NNObjClass* klass;
                klass = (NNObjClass*)object;
                nn_class_destroy(state, klass);
            }
            break;
        case NEON_OBJTYPE_FUNCCLOSURE:
            {
                NNObjFuncClosure* closure;
                closure = (NNObjFuncClosure*)object;
                nn_gcmem_freearray(state, sizeof(NNObjUpvalue*), closure->upvalues, closure->upvalcount);
                /*
                // there may be multiple closures that all reference the same function
                // for this reason, we do not free functions when freeing closures
                */
                nn_gcmem_release(state, object, sizeof(NNObjFuncClosure));
            }
            break;
        case NEON_OBJTYPE_FUNCSCRIPT:
            {
                NNObjFuncScript* function;
                function = (NNObjFuncScript*)object;
                nn_funcscript_destroy(state, function);
                nn_gcmem_release(state, function, sizeof(NNObjFuncScript));
            }
            break;
        case NEON_OBJTYPE_INSTANCE:
            {
                NNObjInstance* instance;
                instance = (NNObjInstance*)object;
                nn_instance_destroy(state, instance);
            }
            break;
        case NEON_OBJTYPE_FUNCNATIVE:
            {
                nn_gcmem_release(state, object, sizeof(NNObjFuncNative));
            }
            break;
        case NEON_OBJTYPE_UPVALUE:
            {
                nn_gcmem_release(state, object, sizeof(NNObjUpvalue));
            }
            break;
        case NEON_OBJTYPE_RANGE:
            {
                nn_gcmem_release(state, object, sizeof(NNObjRange));
            }
            break;
        case NEON_OBJTYPE_STRING:
            {
                NNObjString* string;
                string = (NNObjString*)object;
                nn_string_destroy(state, string);
            }
            break;
        case NEON_OBJTYPE_SWITCH:
            {
                NNObjSwitch* sw;
                sw = (NNObjSwitch*)object;
                nn_table_destroy(sw->table);
                nn_gcmem_release(state, object, sizeof(NNObjSwitch));
            }
            break;
        case NEON_OBJTYPE_USERDATA:
            {
                NNObjUserdata* ptr;
                ptr = (NNObjUserdata*)object;
                if(ptr->ondestroyfn)
                {
                    ptr->ondestroyfn(ptr->pointer);
                }
                nn_gcmem_release(state, object, sizeof(NNObjUserdata));
            }
            break;
        default:
            break;
    }
    
}

void nn_gcmem_markroots(NNState* state)
{
    int i;
    int j;
    NNValue* slot;
    NNObjUpvalue* upvalue;
    NNExceptionFrame* handler;
    for(slot = state->vmstate.stackvalues; slot < &state->vmstate.stackvalues[state->vmstate.stackidx]; slot++)
    {
        nn_gcmem_markvalue(state, *slot);
    }
    for(i = 0; i < (int)state->vmstate.framecount; i++)
    {
        nn_gcmem_markobject(state, (NNObject*)state->vmstate.framevalues[i].closure);
        for(j = 0; j < (int)state->vmstate.framevalues[i].handlercount; j++)
        {
            handler = &state->vmstate.framevalues[i].handlers[j];
            nn_gcmem_markobject(state, (NNObject*)handler->klass);
        }
    }
    for(upvalue = state->vmstate.openupvalues; upvalue != NULL; upvalue = upvalue->next)
    {
        nn_gcmem_markobject(state, (NNObject*)upvalue);
    }
    nn_table_mark(state, state->globals);
    nn_table_mark(state, state->modules);
    nn_gcmem_markobject(state, (NNObject*)state->exceptions.stdexception);
    nn_gcmem_markcompilerroots(state);
}

void nn_gcmem_tracerefs(NNState* state)
{
    NNObject* object;
    while(state->gcstate.graycount > 0)
    {
        state->gcstate.graycount--;
        object = state->gcstate.graystack[state->gcstate.graycount];
        nn_gcmem_blackenobject(state, object);
    }
}

void nn_gcmem_sweep(NNState* state)
{
    NNObject* object;
    NNObject* previous;
    NNObject* unreached;
    previous = NULL;
    object = state->vmstate.linkedobjects;
    while(object != NULL)
    {
        if(object->mark == state->markvalue)
        {
            previous = object;
            object = object->next;
        }
        else
        {
            unreached = object;
            object = object->next;
            if(previous != NULL)
            {
                previous->next = object;
            }
            else
            {
                state->vmstate.linkedobjects = object;
            }
            nn_gcmem_destroyobject(state, unreached);
        }
    }
}

void nn_gcmem_destroylinkedobjects(NNState* state)
{
    NNObject* next;
    NNObject* object;
    object = state->vmstate.linkedobjects;
    while(object != NULL)
    {
        next = object->next;
        nn_gcmem_destroyobject(state, object);
        object = next;
    }
    nn_util_memfree(state, state->gcstate.graystack);
    state->gcstate.graystack = NULL;
}

void nn_gcmem_collectgarbage(NNState* state)
{
    size_t before;
    (void)before;
    #if defined(DEBUG_GC) && DEBUG_GC
    nn_printer_writefmt(state->debugwriter, "GC: gc begins\n");
    before = state->gcstate.bytesallocated;
    #endif
    /*
    //  REMOVE THE NEXT LINE TO DISABLE NESTED nn_gcmem_collectgarbage() POSSIBILITY!
    */
    #if 1
    state->gcstate.nextgc = state->gcstate.bytesallocated;
    #endif
    nn_gcmem_markroots(state);
    nn_gcmem_tracerefs(state);
    nn_table_removewhites(state, state->strings);
    nn_table_removewhites(state, state->modules);
    nn_gcmem_sweep(state);
    state->gcstate.nextgc = state->gcstate.bytesallocated * NEON_CFG_GCHEAPGROWTHFACTOR;
    state->markvalue = !state->markvalue;
    #if defined(DEBUG_GC) && DEBUG_GC
    nn_printer_writefmt(state->debugwriter, "GC: gc ends\n");
    nn_printer_writefmt(state->debugwriter, "GC: collected %zu bytes (from %zu to %zu), next at %zu\n", before - state->gcstate.bytesallocated, before, state->gcstate.bytesallocated, state->gcstate.nextgc);
    #endif
}

NNValue nn_argcheck_vfail(NNArgCheck* ch, const char* srcfile, int srcline, const char* fmt, va_list va)
{
    nn_vm_stackpopn(ch->pvm, ch->argc);
    nn_exceptions_vthrowwithclass(ch->pvm, ch->pvm->exceptions.argumenterror, srcfile, srcline, fmt, va);
    return nn_value_makebool(false);
}

NNValue nn_argcheck_fail(NNArgCheck* ch, const char* srcfile, int srcline, const char* fmt, ...)
{
    NNValue v;
    va_list va;
    va_start(va, fmt);
    v = nn_argcheck_vfail(ch, srcfile, srcline, fmt, va);
    va_end(va);
    return v;
}

void nn_argcheck_init(NNState* state, NNArgCheck* ch, NNArguments* args)
{
    ch->pvm = state;
    ch->name = args->name;
    ch->argc = args->count;
    ch->argv = args->args;
}

void nn_dbg_disasmblob(NNPrinter* pr, NNBlob* blob, const char* name)
{
    int offset;
    nn_printer_writefmt(pr, "== compiled '%s' [[\n", name);
    for(offset = 0; offset < blob->count;)
    {
        offset = nn_dbg_printinstructionat(pr, blob, offset);
    }
    nn_printer_writefmt(pr, "]]\n");
}

void nn_dbg_printinstrname(NNPrinter* pr, const char* name)
{
    nn_printer_writefmt(pr, "%s%-16s%s ", nn_util_color(NEON_COLOR_RED), name, nn_util_color(NEON_COLOR_RESET));
}

int nn_dbg_printsimpleinstr(NNPrinter* pr, const char* name, int offset)
{
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "\n");
    return offset + 1;
}

int nn_dbg_printconstinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    uint16_t constant;
    constant = (blob->instrucs[offset + 1].code << 8) | blob->instrucs[offset + 2].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "%8d ", constant);
    nn_printer_printvalue(pr, blob->constants->listitems[constant], true, false);
    nn_printer_writefmt(pr, "\n");
    return offset + 3;
}

int nn_dbg_printpropertyinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    const char* proptn;
    uint16_t constant;
    constant = (blob->instrucs[offset + 1].code << 8) | blob->instrucs[offset + 2].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "%8d ", constant);
    nn_printer_printvalue(pr, blob->constants->listitems[constant], true, false);
    proptn = "";
    if(blob->instrucs[offset + 3].code == 1)
    {
        proptn = "static";
    }
    nn_printer_writefmt(pr, " (%s)", proptn);
    nn_printer_writefmt(pr, "\n");
    return offset + 4;
}

int nn_dbg_printshortinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    uint16_t slot;
    slot = (blob->instrucs[offset + 1].code << 8) | blob->instrucs[offset + 2].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "%8d\n", slot);
    return offset + 3;
}

int nn_dbg_printbyteinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    uint8_t slot;
    slot = blob->instrucs[offset + 1].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "%8d\n", slot);
    return offset + 2;
}

int nn_dbg_printjumpinstr(NNPrinter* pr, const char* name, int sign, NNBlob* blob, int offset)
{
    uint16_t jump;
    jump = (uint16_t)(blob->instrucs[offset + 1].code << 8);
    jump |= blob->instrucs[offset + 2].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "%8d -> %d\n", offset, offset + 3 + sign * jump);
    return offset + 3;
}

int nn_dbg_printtryinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    uint16_t finally;
    uint16_t type;
    uint16_t address;
    type = (uint16_t)(blob->instrucs[offset + 1].code << 8);
    type |= blob->instrucs[offset + 2].code;
    address = (uint16_t)(blob->instrucs[offset + 3].code << 8);
    address |= blob->instrucs[offset + 4].code;
    finally = (uint16_t)(blob->instrucs[offset + 5].code << 8);
    finally |= blob->instrucs[offset + 6].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "%8d -> %d, %d\n", type, address, finally);
    return offset + 7;
}

int nn_dbg_printinvokeinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    uint16_t constant;
    uint8_t argcount;
    constant = (uint16_t)(blob->instrucs[offset + 1].code << 8);
    constant |= blob->instrucs[offset + 2].code;
    argcount = blob->instrucs[offset + 3].code;
    nn_dbg_printinstrname(pr, name);
    nn_printer_writefmt(pr, "(%d args) %8d ", argcount, constant);
    nn_printer_printvalue(pr, blob->constants->listitems[constant], true, false);
    nn_printer_writefmt(pr, "\n");
    return offset + 4;
}

const char* nn_dbg_op2str(uint8_t instruc)
{
    switch(instruc)
    {
        case NEON_OP_GLOBALDEFINE: return "NEON_OP_GLOBALDEFINE";
        case NEON_OP_GLOBALGET: return "NEON_OP_GLOBALGET";
        case NEON_OP_GLOBALSET: return "NEON_OP_GLOBALSET";
        case NEON_OP_LOCALGET: return "NEON_OP_LOCALGET";
        case NEON_OP_LOCALSET: return "NEON_OP_LOCALSET";
        case NEON_OP_FUNCARGSET: return "NEON_OP_FUNCARGSET";
        case NEON_OP_FUNCARGGET: return "NEON_OP_FUNCARGGET";
        case NEON_OP_UPVALUEGET: return "NEON_OP_UPVALUEGET";
        case NEON_OP_UPVALUESET: return "NEON_OP_UPVALUESET";
        case NEON_OP_UPVALUECLOSE: return "NEON_OP_UPVALUECLOSE";
        case NEON_OP_PROPERTYGET: return "NEON_OP_PROPERTYGET";
        case NEON_OP_PROPERTYGETSELF: return "NEON_OP_PROPERTYGETSELF";
        case NEON_OP_PROPERTYSET: return "NEON_OP_PROPERTYSET";
        case NEON_OP_JUMPIFFALSE: return "NEON_OP_JUMPIFFALSE";
        case NEON_OP_JUMPNOW: return "NEON_OP_JUMPNOW";
        case NEON_OP_LOOP: return "NEON_OP_LOOP";
        case NEON_OP_EQUAL: return "NEON_OP_EQUAL";
        case NEON_OP_PRIMGREATER: return "NEON_OP_PRIMGREATER";
        case NEON_OP_PRIMLESSTHAN: return "NEON_OP_PRIMLESSTHAN";
        case NEON_OP_PUSHEMPTY: return "NEON_OP_PUSHEMPTY";
        case NEON_OP_PUSHNULL: return "NEON_OP_PUSHNULL";
        case NEON_OP_PUSHTRUE: return "NEON_OP_PUSHTRUE";
        case NEON_OP_PUSHFALSE: return "NEON_OP_PUSHFALSE";
        case NEON_OP_PRIMADD: return "NEON_OP_PRIMADD";
        case NEON_OP_PRIMSUBTRACT: return "NEON_OP_PRIMSUBTRACT";
        case NEON_OP_PRIMMULTIPLY: return "NEON_OP_PRIMMULTIPLY";
        case NEON_OP_PRIMDIVIDE: return "NEON_OP_PRIMDIVIDE";
        case NEON_OP_PRIMFLOORDIVIDE: return "NEON_OP_PRIMFLOORDIVIDE";
        case NEON_OP_PRIMMODULO: return "NEON_OP_PRIMMODULO";
        case NEON_OP_PRIMPOW: return "NEON_OP_PRIMPOW";
        case NEON_OP_PRIMNEGATE: return "NEON_OP_PRIMNEGATE";
        case NEON_OP_PRIMNOT: return "NEON_OP_PRIMNOT";
        case NEON_OP_PRIMBITNOT: return "NEON_OP_PRIMBITNOT";
        case NEON_OP_PRIMAND: return "NEON_OP_PRIMAND";
        case NEON_OP_PRIMOR: return "NEON_OP_PRIMOR";
        case NEON_OP_PRIMBITXOR: return "NEON_OP_PRIMBITXOR";
        case NEON_OP_PRIMSHIFTLEFT: return "NEON_OP_PRIMSHIFTLEFT";
        case NEON_OP_PRIMSHIFTRIGHT: return "NEON_OP_PRIMSHIFTRIGHT";
        case NEON_OP_PUSHONE: return "NEON_OP_PUSHONE";
        case NEON_OP_PUSHCONSTANT: return "NEON_OP_PUSHCONSTANT";
        case NEON_OP_ECHO: return "NEON_OP_ECHO";
        case NEON_OP_POPONE: return "NEON_OP_POPONE";
        case NEON_OP_DUPONE: return "NEON_OP_DUPONE";
        case NEON_OP_POPN: return "NEON_OP_POPN";
        case NEON_OP_ASSERT: return "NEON_OP_ASSERT";
        case NEON_OP_EXTHROW: return "NEON_OP_EXTHROW";
        case NEON_OP_MAKECLOSURE: return "NEON_OP_MAKECLOSURE";
        case NEON_OP_CALLFUNCTION: return "NEON_OP_CALLFUNCTION";
        case NEON_OP_CALLMETHOD: return "NEON_OP_CALLMETHOD";
        case NEON_OP_CLASSINVOKETHIS: return "NEON_OP_CLASSINVOKETHIS";
        case NEON_OP_CLASSGETTHIS: return "NEON_OP_CLASSGETTHIS";
        case NEON_OP_RETURN: return "NEON_OP_RETURN";
        case NEON_OP_MAKECLASS: return "NEON_OP_MAKECLASS";
        case NEON_OP_MAKEMETHOD: return "NEON_OP_MAKEMETHOD";
        case NEON_OP_CLASSPROPERTYDEFINE: return "NEON_OP_CLASSPROPERTYDEFINE";
        case NEON_OP_CLASSINHERIT: return "NEON_OP_CLASSINHERIT";
        case NEON_OP_CLASSGETSUPER: return "NEON_OP_CLASSGETSUPER";
        case NEON_OP_CLASSINVOKESUPER: return "NEON_OP_CLASSINVOKESUPER";
        case NEON_OP_CLASSINVOKESUPERSELF: return "NEON_OP_CLASSINVOKESUPERSELF";
        case NEON_OP_MAKERANGE: return "NEON_OP_MAKERANGE";
        case NEON_OP_MAKEARRAY: return "NEON_OP_MAKEARRAY";
        case NEON_OP_MAKEDICT: return "NEON_OP_MAKEDICT";
        case NEON_OP_INDEXGET: return "NEON_OP_INDEXGET";
        case NEON_OP_INDEXGETRANGED: return "NEON_OP_INDEXGETRANGED";
        case NEON_OP_INDEXSET: return "NEON_OP_INDEXSET";
        case NEON_OP_IMPORTIMPORT: return "NEON_OP_IMPORTIMPORT";
        case NEON_OP_EXTRY: return "NEON_OP_EXTRY";
        case NEON_OP_EXPOPTRY: return "NEON_OP_EXPOPTRY";
        case NEON_OP_EXPUBLISHTRY: return "NEON_OP_EXPUBLISHTRY";
        case NEON_OP_STRINGIFY: return "NEON_OP_STRINGIFY";
        case NEON_OP_SWITCH: return "NEON_OP_SWITCH";
        case NEON_OP_TYPEOF: return "NEON_OP_TYPEOF";
        case NEON_OP_BREAK_PL: return "NEON_OP_BREAK_PL";
        default:
            break;
    }
    return "<?unknown?>";
}

int nn_dbg_printclosureinstr(NNPrinter* pr, const char* name, NNBlob* blob, int offset)
{
    int j;
    int islocal;
    uint16_t index;
    uint16_t constant;
    const char* locn;
    NNObjFuncScript* function;
    offset++;
    constant = blob->instrucs[offset++].code << 8;
    constant |= blob->instrucs[offset++].code;
    nn_printer_writefmt(pr, "%-16s %8d ", name, constant);
    nn_printer_printvalue(pr, blob->constants->listitems[constant], true, false);
    nn_printer_writefmt(pr, "\n");
    function = nn_value_asfuncscript(blob->constants->listitems[constant]);
    for(j = 0; j < function->upvalcount; j++)
    {
        islocal = blob->instrucs[offset++].code;
        index = blob->instrucs[offset++].code << 8;
        index |= blob->instrucs[offset++].code;
        locn = "upvalue";
        if(islocal)
        {
            locn = "local";
        }
        nn_printer_writefmt(pr, "%04d      |                     %s %d\n", offset - 3, locn, (int)index);
    }
    return offset;
}

int nn_dbg_printinstructionat(NNPrinter* pr, NNBlob* blob, int offset)
{
    uint8_t instruction;
    const char* opname;
    nn_printer_writefmt(pr, "%08d ", offset);
    if(offset > 0 && blob->instrucs[offset].srcline == blob->instrucs[offset - 1].srcline)
    {
        nn_printer_writefmt(pr, "       | ");
    }
    else
    {
        nn_printer_writefmt(pr, "%8d ", blob->instrucs[offset].srcline);
    }
    instruction = blob->instrucs[offset].code;
    opname = nn_dbg_op2str(instruction);
    switch(instruction)
    {
        case NEON_OP_JUMPIFFALSE:
            return nn_dbg_printjumpinstr(pr, opname, 1, blob, offset);
        case NEON_OP_JUMPNOW:
            return nn_dbg_printjumpinstr(pr, opname, 1, blob, offset);
        case NEON_OP_EXTRY:
            return nn_dbg_printtryinstr(pr, opname, blob, offset);
        case NEON_OP_LOOP:
            return nn_dbg_printjumpinstr(pr, opname, -1, blob, offset);
        case NEON_OP_GLOBALDEFINE:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_GLOBALGET:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_GLOBALSET:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_LOCALGET:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_LOCALSET:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_FUNCARGGET:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_FUNCARGSET:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_PROPERTYGET:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_PROPERTYGETSELF:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_PROPERTYSET:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_UPVALUEGET:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_UPVALUESET:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_EXPOPTRY:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_EXPUBLISHTRY:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PUSHCONSTANT:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_EQUAL:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMGREATER:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMLESSTHAN:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PUSHEMPTY:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PUSHNULL:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PUSHTRUE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PUSHFALSE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMADD:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMSUBTRACT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMMULTIPLY:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMDIVIDE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMFLOORDIVIDE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMMODULO:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMPOW:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMNEGATE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMNOT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMBITNOT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMAND:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMOR:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMBITXOR:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMSHIFTLEFT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PRIMSHIFTRIGHT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_PUSHONE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_IMPORTIMPORT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_TYPEOF:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_ECHO:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_STRINGIFY:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_EXTHROW:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_POPONE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_UPVALUECLOSE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_DUPONE:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_ASSERT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_POPN:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
            /* non-user objects... */
        case NEON_OP_SWITCH:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
            /* data container manipulators */
        case NEON_OP_MAKERANGE:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_MAKEARRAY:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_MAKEDICT:
            return nn_dbg_printshortinstr(pr, opname, blob, offset);
        case NEON_OP_INDEXGET:
            return nn_dbg_printbyteinstr(pr, opname, blob, offset);
        case NEON_OP_INDEXGETRANGED:
            return nn_dbg_printbyteinstr(pr, opname, blob, offset);
        case NEON_OP_INDEXSET:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_MAKECLOSURE:
            return nn_dbg_printclosureinstr(pr, opname, blob, offset);
        case NEON_OP_CALLFUNCTION:
            return nn_dbg_printbyteinstr(pr, opname, blob, offset);
        case NEON_OP_CALLMETHOD:
            return nn_dbg_printinvokeinstr(pr, opname, blob, offset);
        case NEON_OP_CLASSINVOKETHIS:
            return nn_dbg_printinvokeinstr(pr, opname, blob, offset);
        case NEON_OP_RETURN:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_CLASSGETTHIS:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_MAKECLASS:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_MAKEMETHOD:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_CLASSPROPERTYDEFINE:
            return nn_dbg_printpropertyinstr(pr, opname, blob, offset);
        case NEON_OP_CLASSGETSUPER:
            return nn_dbg_printconstinstr(pr, opname, blob, offset);
        case NEON_OP_CLASSINHERIT:
            return nn_dbg_printsimpleinstr(pr, opname, offset);
        case NEON_OP_CLASSINVOKESUPER:
            return nn_dbg_printinvokeinstr(pr, opname, blob, offset);
        case NEON_OP_CLASSINVOKESUPERSELF:
            return nn_dbg_printbyteinstr(pr, opname, blob, offset);
        default:
            {
                nn_printer_writefmt(pr, "unknown opcode %d\n", instruction);
            }
            break;
    }
    return offset + 1;
}

void nn_blob_init(NNState* state, NNBlob* blob)
{
    blob->count = 0;
    blob->capacity = 0;
    blob->instrucs = NULL;
    blob->constants = nn_vallist_make(state);
    blob->argdefvals = nn_vallist_make(state);
}

void nn_blob_push(NNState* state, NNBlob* blob, NNInstruction ins)
{
    int oldcapacity;
    if(blob->capacity < blob->count + 1)
    {
        oldcapacity = blob->capacity;
        blob->capacity = GROW_CAPACITY(oldcapacity);
        blob->instrucs = (NNInstruction*)nn_gcmem_growarray(state, sizeof(NNInstruction), blob->instrucs, oldcapacity, blob->capacity);
    }
    blob->instrucs[blob->count] = ins;
    blob->count++;
}

void nn_blob_destroy(NNState* state, NNBlob* blob)
{
    if(blob->instrucs != NULL)
    {
        nn_gcmem_freearray(state, sizeof(NNInstruction), blob->instrucs, blob->capacity);
    }
    nn_vallist_destroy(blob->constants);
    nn_vallist_destroy(blob->argdefvals);
}

int nn_blob_pushconst(NNState* state, NNBlob* blob, NNValue value)
{
    (void)state;
    nn_vallist_push(blob->constants, value);
    return blob->constants->listcount - 1;
}

int nn_blob_pushargdefval(NNState* state, NNBlob* blob, NNValue value)
{
    (void)state;
    nn_vallist_push(blob->argdefvals, value);
    return blob->argdefvals->listcount - 1;
}

NNProperty nn_property_makewithpointer(NNState* state, NNValue val, NNFieldType type)
{
    NNProperty vf;
    (void)state;
    memset(&vf, 0, sizeof(NNProperty));
    vf.type = type;
    vf.value = val;
    vf.havegetset = false;
    return vf;
}

NNProperty nn_property_makewithgetset(NNState* state, NNValue val, NNValue getter, NNValue setter, NNFieldType type)
{
    bool getisfn;
    bool setisfn;
    NNProperty np;
    np = nn_property_makewithpointer(state, val, type);
    setisfn = nn_value_iscallable(setter);
    getisfn = nn_value_iscallable(getter);
    if(getisfn || setisfn)
    {
        np.getset.setter = setter;
        np.getset.getter = getter;
    }
    return np;
}

NNProperty nn_property_make(NNState* state, NNValue val, NNFieldType type)
{
    return nn_property_makewithpointer(state, val, type);
}

NNHashTable* nn_table_make(NNState* state)
{
    NNHashTable* table;
    table = (NNHashTable*)nn_gcmem_allocate(state, sizeof(NNHashTable), 1);
    if(table == NULL)
    {
        return NULL;
    }
    table->pvm = state;
    table->active = true;
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
    return table;
}

void nn_table_destroy(NNHashTable* table)
{
    NNState* state;
    if(table != NULL)
    {
        state = table->pvm;
        nn_gcmem_freearray(state, sizeof(NNHashEntry), table->entries, table->capacity);
        memset(table, 0, sizeof(NNHashTable));
        nn_gcmem_release(state, table, sizeof(NNHashTable));
    }
}

NNHashEntry* nn_table_findentrybyvalue(NNHashTable* table, NNHashEntry* entries, int capacity, NNValue key)
{
    uint32_t hash;
    uint32_t index;
    NNState* state;
    NNHashEntry* entry;
    NNHashEntry* tombstone;
    state = table->pvm;
    hash = nn_value_hashvalue(key);
    #if defined(DEBUG_TABLE) && DEBUG_TABLE
    fprintf(stderr, "looking for key ");
    nn_printer_printvalue(state->debugwriter, key, true, false);
    fprintf(stderr, " with hash %u in table...\n", hash);
    #endif
    index = hash & (capacity - 1);
    tombstone = NULL;
    while(true)
    {
        entry = &entries[index];
        if(nn_value_isempty(entry->key))
        {
            if(nn_value_isnull(entry->value.value))
            {
                /* empty entry */
                if(tombstone != NULL)
                {
                    return tombstone;
                }
                else
                {
                    return entry;
                }
            }
            else
            {
                /* we found a tombstone. */
                if(tombstone == NULL)
                {
                    tombstone = entry;
                }
            }
        }
        else if(nn_value_compare(state, key, entry->key))
        {
            return entry;
        }
        index = (index + 1) & (capacity - 1);
    }
    return NULL;
}

NNHashEntry* nn_table_findentrybystr(NNHashTable* table, NNHashEntry* entries, int capacity, NNValue valkey, const char* kstr, size_t klen, uint32_t hash)
{
    bool havevalhash;
    uint32_t index;
    uint32_t valhash;
    NNObjString* entoskey;
    NNHashEntry* entry;
    NNHashEntry* tombstone;
    NNState* state;
    state = table->pvm;
    (void)valhash;
    (void)havevalhash;
    #if defined(DEBUG_TABLE) && DEBUG_TABLE
    fprintf(stderr, "looking for key ");
    nn_printer_printvalue(state->debugwriter, key, true, false);
    fprintf(stderr, " with hash %u in table...\n", hash);
    #endif
    valhash = 0;
    havevalhash = false;
    index = hash & (capacity - 1);
    tombstone = NULL;
    while(true)
    {
        entry = &entries[index];
        if(nn_value_isempty(entry->key))
        {
            if(nn_value_isnull(entry->value.value))
            {
                /* empty entry */
                if(tombstone != NULL)
                {
                    return tombstone;
                }
                else
                {
                    return entry;
                }
            }
            else
            {
                /* we found a tombstone. */
                if(tombstone == NULL)
                {
                    tombstone = entry;
                }
            }
        }
        if(nn_value_isstring(entry->key))
        {
            entoskey = nn_value_asstring(entry->key);
            if(entoskey->sbuf->length == klen)
            {
                if(memcmp(kstr, entoskey->sbuf->data, klen) == 0)
                {
                    return entry;
                }
            }
        }
        else
        {
            if(!nn_value_isempty(valkey))
            {
                if(nn_value_compare(state, valkey, entry->key))
                {
                    return entry;
                }
            }
        }
        index = (index + 1) & (capacity - 1);
    }
    return NULL;
}

NNProperty* nn_table_getfieldbyvalue(NNHashTable* table, NNValue key)
{
    NNState* state;
    NNHashEntry* entry;
    (void)state;
    state = table->pvm;
    if(table->count == 0 || table->entries == NULL)
    {
        return NULL;
    }
    #if defined(DEBUG_TABLE) && DEBUG_TABLE
    fprintf(stderr, "getting entry with hash %u...\n", nn_value_hashvalue(key));
    #endif
    entry = nn_table_findentrybyvalue(table, table->entries, table->capacity, key);
    if(nn_value_isempty(entry->key) || nn_value_isnull(entry->key))
    {
        return NULL;
    }
    #if defined(DEBUG_TABLE) && DEBUG_TABLE
    fprintf(stderr, "found entry for hash %u == ", nn_value_hashvalue(entry->key));
    nn_printer_printvalue(state->debugwriter, entry->value.value, true, false);
    fprintf(stderr, "\n");
    #endif
    return &entry->value;
}

NNProperty* nn_table_getfieldbystr(NNHashTable* table, NNValue valkey, const char* kstr, size_t klen, uint32_t hash)
{
    NNState* state;
    NNHashEntry* entry;
    (void)state;
    state = table->pvm;
    if(table->count == 0 || table->entries == NULL)
    {
        return NULL;
    }
    #if defined(DEBUG_TABLE) && DEBUG_TABLE
    fprintf(stderr, "getting entry with hash %u...\n", nn_value_hashvalue(key));
    #endif
    entry = nn_table_findentrybystr(table, table->entries, table->capacity, valkey, kstr, klen, hash);
    if(nn_value_isempty(entry->key) || nn_value_isnull(entry->key))
    {
        return NULL;
    }
    #if defined(DEBUG_TABLE) && DEBUG_TABLE
    fprintf(stderr, "found entry for hash %u == ", nn_value_hashvalue(entry->key));
    nn_printer_printvalue(state->debugwriter, entry->value.value, true, false);
    fprintf(stderr, "\n");
    #endif
    return &entry->value;
}

NNProperty* nn_table_getfieldbyostr(NNHashTable* table, NNObjString* str)
{
    return nn_table_getfieldbystr(table, nn_value_makeempty(), str->sbuf->data, str->sbuf->length, str->hash);
}

NNProperty* nn_table_getfieldbycstr(NNHashTable* table, const char* kstr)
{
    size_t klen;
    uint32_t hash;
    klen = strlen(kstr);
    hash = nn_util_hashstring(kstr, klen);
    return nn_table_getfieldbystr(table, nn_value_makeempty(), kstr, klen, hash);
}

NNProperty* nn_table_getfield(NNHashTable* table, NNValue key)
{
    NNObjString* oskey;
    if(nn_value_isstring(key))
    {
        oskey = nn_value_asstring(key);
        return nn_table_getfieldbystr(table, key, oskey->sbuf->data, oskey->sbuf->length, oskey->hash);
    }
    return nn_table_getfieldbyvalue(table, key);
}

bool nn_table_get(NNHashTable* table, NNValue key, NNValue* value)
{
    NNProperty* field;
    field = nn_table_getfield(table, key);
    if(field != NULL)
    {
        *value = field->value;
        return true;
    }
    return false;
}

void nn_table_adjustcapacity(NNHashTable* table, int capacity)
{
    int i;
    NNState* state;
    NNHashEntry* dest;
    NNHashEntry* entry;
    NNHashEntry* entries;
    state = table->pvm;
    entries = (NNHashEntry*)nn_gcmem_allocate(state, sizeof(NNHashEntry), capacity);
    for(i = 0; i < capacity; i++)
    {
        entries[i].key = nn_value_makeempty();
        entries[i].value = nn_property_make(state, nn_value_makenull(), NEON_PROPTYPE_VALUE);
    }
    /* repopulate buckets */
    table->count = 0;
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(nn_value_isempty(entry->key))
        {
            continue;
        }
        dest = nn_table_findentrybyvalue(table, entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    /* free the old entries... */
    nn_gcmem_freearray(state, sizeof(NNHashEntry), table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool nn_table_setwithtype(NNHashTable* table, NNValue key, NNValue value, NNFieldType ftyp, bool keyisstring)
{
    bool isnew;
    int capacity;
    NNState* state;
    NNHashEntry* entry;
    (void)keyisstring;
    state = table->pvm;
    if(table->count + 1 > table->capacity * NEON_CFG_MAXTABLELOAD)
    {
        capacity = GROW_CAPACITY(table->capacity);
        nn_table_adjustcapacity(table, capacity);
    }
    entry = nn_table_findentrybyvalue(table, table->entries, table->capacity, key);
    isnew = nn_value_isempty(entry->key);
    if(isnew && nn_value_isnull(entry->value.value))
    {
        table->count++;
    }
    /* overwrites existing entries. */
    entry->key = key;
    entry->value = nn_property_make(state, value, ftyp);
    return isnew;
}

bool nn_table_set(NNHashTable* table, NNValue key, NNValue value)
{
    return nn_table_setwithtype(table, key, value, NEON_PROPTYPE_VALUE, nn_value_isstring(key));
}

bool nn_table_setcstrwithtype(NNHashTable* table, const char* cstrkey, NNValue value, NNFieldType ftype)
{
    NNObjString* os;
    NNState* state;
    state = table->pvm;
    os = nn_string_copycstr(state, cstrkey);
    return nn_table_setwithtype(table, nn_value_fromobject(os), value, ftype, true);
}

bool nn_table_setcstr(NNHashTable* table, const char* cstrkey, NNValue value)
{
    return nn_table_setcstrwithtype(table, cstrkey, value, NEON_PROPTYPE_VALUE);
}

bool nn_table_delete(NNHashTable* table, NNValue key)
{
    NNHashEntry* entry;
    if(table->count == 0)
    {
        return false;
    }
    /* find the entry */
    entry = nn_table_findentrybyvalue(table, table->entries, table->capacity, key);
    if(nn_value_isempty(entry->key))
    {
        return false;
    }
    /* place a tombstone in the entry. */
    entry->key = nn_value_makeempty();
    entry->value = nn_property_make(table->pvm, nn_value_makebool(true), NEON_PROPTYPE_VALUE);
    return true;
}

void nn_table_addall(NNHashTable* from, NNHashTable* to)
{
    int i;
    NNHashEntry* entry;
    for(i = 0; i < from->capacity; i++)
    {
        entry = &from->entries[i];
        if(!nn_value_isempty(entry->key))
        {
            nn_table_setwithtype(to, entry->key, entry->value.value, entry->value.type, false);
        }
    }
}

void nn_table_importall(NNHashTable* from, NNHashTable* to)
{
    int i;
    NNHashEntry* entry;
    for(i = 0; i < (int)from->capacity; i++)
    {
        entry = &from->entries[i];
        if(!nn_value_isempty(entry->key) && !nn_value_ismodule(entry->value.value))
        {
            /* Don't import private values */
            if(nn_value_isstring(entry->key) && nn_value_asstring(entry->key)->sbuf->data[0] == '_')
            {
                continue;
            }
            nn_table_setwithtype(to, entry->key, entry->value.value, entry->value.type, false);
        }
    }
}

void nn_table_copy(NNHashTable* from, NNHashTable* to)
{
    int i;
    NNState* state;
    NNHashEntry* entry;
    state = from->pvm;
    for(i = 0; i < (int)from->capacity; i++)
    {
        entry = &from->entries[i];
        if(!nn_value_isempty(entry->key))
        {
            nn_table_setwithtype(to, entry->key, nn_value_copyvalue(state, entry->value.value), entry->value.type, false);
        }
    }
}

NNObjString* nn_table_findstring(NNHashTable* table, const char* chars, size_t length, uint32_t hash)
{
    size_t slen;
    uint32_t index;
    const char* sdata;
    NNHashEntry* entry;
    NNObjString* string;
    if(table->count == 0)
    {
        return NULL;
    }
    index = hash & (table->capacity - 1);
    while(true)
    {
        entry = &table->entries[index];
        if(nn_value_isempty(entry->key))
        {
            /*
            // stop if we find an empty non-tombstone entry
            //if (nn_value_isnull(entry->value))
            */
            {
                return NULL;
            }
        }
        string = nn_value_asstring(entry->key);
        slen = string->sbuf->length;
        sdata = string->sbuf->data;
        if((slen == length) && (string->hash == hash) && memcmp(sdata, chars, length) == 0)
        {
            /* we found it */
            return string;
        }
        index = (index + 1) & (table->capacity - 1);
    }
}

NNValue nn_table_findkey(NNHashTable* table, NNValue value)
{
    int i;
    NNHashEntry* entry;
    for(i = 0; i < (int)table->capacity; i++)
    {
        entry = &table->entries[i];
        if(!nn_value_isnull(entry->key) && !nn_value_isempty(entry->key))
        {
            if(nn_value_compare(table->pvm, entry->value.value, value))
            {
                return entry->key;
            }
        }
    }
    return nn_value_makenull();
}

NNObjArray* nn_table_getkeys(NNHashTable* table)
{
    int i;
    NNState* state;
    NNObjArray* list;
    NNHashEntry* entry;
    state = table->pvm;
    list = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(!nn_value_isnull(entry->key) && !nn_value_isempty(entry->key))
        {
            nn_vallist_push(list->varray, entry->key);
        }
    }
    return list;
}

void nn_table_print(NNState* state, NNPrinter* pr, NNHashTable* table, const char* name)
{
    int i;
    NNHashEntry* entry;
    (void)state;
    nn_printer_writefmt(pr, "<HashTable of %s : {\n", name);
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(!nn_value_isempty(entry->key))
        {
            nn_printer_printvalue(pr, entry->key, true, true);
            nn_printer_writefmt(pr, ": ");
            nn_printer_printvalue(pr, entry->value.value, true, true);
            if(i != table->capacity - 1)
            {
                nn_printer_writefmt(pr, ",\n");
            }
        }
    }
    nn_printer_writefmt(pr, "}>\n");
}

void nn_table_mark(NNState* state, NNHashTable* table)
{
    int i;
    NNHashEntry* entry;
    if(table == NULL)
    {
        return;
    }
    if(!table->active)
    {
        nn_state_warn(state, "trying to mark inactive hashtable <%p>!", table);
        return;
    }
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(entry != NULL)
        {
            nn_gcmem_markvalue(state, entry->key);
            nn_gcmem_markvalue(state, entry->value.value);
        }
    }
}

void nn_table_removewhites(NNState* state, NNHashTable* table)
{
    int i;
    NNHashEntry* entry;
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(nn_value_isobject(entry->key) && nn_value_asobject(entry->key)->mark != state->markvalue)
        {
            nn_table_delete(table, entry->key);
        }
    }
}


void nn_printer_initvars(NNState* state, NNPrinter* pr, NNPrMode mode)
{
    pr->pvm = state;
    pr->fromstack = false;
    pr->wrmode = NEON_PRMODE_UNDEFINED;
    pr->shouldclose = false;
    pr->shouldflush = false;
    pr->stringtaken = false;
    pr->shortenvalues = false;
    pr->maxvallength = 15;
    pr->strbuf = NULL;
    pr->handle = NULL;
    pr->wrmode = mode;
}

NNPrinter* nn_printer_makeundefined(NNState* state, NNPrMode mode)
{
    NNPrinter* pr;
    (void)state;
    pr = (NNPrinter*)nn_gcmem_allocate(state, sizeof(NNPrinter), 1);
    if(!pr)
    {
        fprintf(stderr, "cannot allocate NNPrinter\n");
        return NULL;
    }
    nn_printer_initvars(state, pr, mode);
    return pr;
}

NNPrinter* nn_printer_makeio(NNState* state, FILE* fh, bool shouldclose)
{
    NNPrinter* pr;
    pr = nn_printer_makeundefined(state, NEON_PRMODE_FILE);
    pr->handle = fh;
    pr->shouldclose = shouldclose;
    return pr;
}

NNPrinter* nn_printer_makestring(NNState* state)
{
    NNPrinter* pr;
    pr = nn_printer_makeundefined(state, NEON_PRMODE_STRING);
    pr->strbuf = dyn_strbuf_makeempty(0);
    return pr;
}

void nn_printer_makestackio(NNState* state, NNPrinter* pr, FILE* fh, bool shouldclose)
{
    nn_printer_initvars(state, pr, NEON_PRMODE_FILE);
    pr->fromstack = true;
    pr->handle = fh;
    pr->shouldclose = shouldclose;
}

void nn_printer_makestackstring(NNState* state, NNPrinter* pr)
{
    nn_printer_initvars(state, pr, NEON_PRMODE_STRING);
    pr->fromstack = true;
    pr->wrmode = NEON_PRMODE_STRING;
    pr->strbuf = dyn_strbuf_makeempty(0);
}

void nn_printer_destroy(NNPrinter* pr)
{
    NNState* state;
    (void)state;
    if(pr == NULL)
    {
        return;
    }
    if(pr->wrmode == NEON_PRMODE_UNDEFINED)
    {
        return;
    }
    /*fprintf(stderr, "nn_printer_destroy: pr->wrmode=%d\n", pr->wrmode);*/
    state = pr->pvm;
    if(pr->wrmode == NEON_PRMODE_STRING)
    {
        if(!pr->stringtaken)
        {
            dyn_strbuf_destroy(pr->strbuf);
        }
    }
    else if(pr->wrmode == NEON_PRMODE_FILE)
    {
        if(pr->shouldclose)
        {
            #if 0
            fclose(pr->handle);
            #endif
        }
    }
    if(!pr->fromstack)
    {
        nn_util_memfree(state, pr);
        pr = NULL;
    }
}

NNObjString* nn_printer_takestring(NNPrinter* pr)
{
    uint32_t hash;
    NNState* state;
    NNObjString* os;
    state = pr->pvm;
    hash = nn_util_hashstring(pr->strbuf->data, pr->strbuf->length);
    os = nn_string_makefromstrbuf(state, pr->strbuf, hash);
    pr->stringtaken = true;
    return os;
}

bool nn_printer_writestringl(NNPrinter* pr, const char* estr, size_t elen)
{
    if(pr->wrmode == NEON_PRMODE_FILE)
    {
        fwrite(estr, sizeof(char), elen, pr->handle);
        if(pr->shouldflush)
        {
            fflush(pr->handle);
        }
    }
    else if(pr->wrmode == NEON_PRMODE_STRING)
    {
        dyn_strbuf_appendstrn(pr->strbuf, estr, elen);
    }
    else
    {
        return false;
    }
    return true;
}

bool nn_printer_writestring(NNPrinter* pr, const char* estr)
{
    return nn_printer_writestringl(pr, estr, strlen(estr));
}

bool nn_printer_writechar(NNPrinter* pr, int b)
{
    char ch;
    if(pr->wrmode == NEON_PRMODE_STRING)
    {
        ch = b;
        nn_printer_writestringl(pr, &ch, 1);
    }
    else if(pr->wrmode == NEON_PRMODE_FILE)
    {
        fputc(b, pr->handle);
        if(pr->shouldflush)
        {
            fflush(pr->handle);
        }
    }
    return true;
}

bool nn_printer_writeescapedchar(NNPrinter* pr, int ch)
{
    switch(ch)
    {
        case '\'':
            {
                nn_printer_writestring(pr, "\\\'");
            }
            break;
        case '\"':
            {
                nn_printer_writestring(pr, "\\\"");
            }
            break;
        case '\\':
            {
                nn_printer_writestring(pr, "\\\\");
            }
            break;
        case '\b':
            {
                nn_printer_writestring(pr, "\\b");
            }
            break;
        case '\f':
            {
                nn_printer_writestring(pr, "\\f");
            }
            break;
        case '\n':
            {
                nn_printer_writestring(pr, "\\n");
            }
            break;
        case '\r':
            {
                nn_printer_writestring(pr, "\\r");
            }
            break;
        case '\t':
            {
                nn_printer_writestring(pr, "\\t");
            }
            break;
        case 0:
            {
                nn_printer_writestring(pr, "\\0");
            }
            break;
        default:
            {
                nn_printer_writefmt(pr, "\\x%02x", (unsigned char)ch);
            }
            break;
    }
    return true;
}

bool nn_printer_writequotedstring(NNPrinter* pr, const char* str, size_t len, bool withquot)
{
    int bch;
    size_t i;
    bch = 0;
    if(withquot)
    {
        nn_printer_writechar(pr, '"');
    }
    for(i = 0; i < len; i++)
    {
        bch = str[i];
        if((bch < 32) || (bch > 127) || (bch == '\"') || (bch == '\\'))
        {
            nn_printer_writeescapedchar(pr, bch);
        }
        else
        {
            nn_printer_writechar(pr, bch);
        }
    }
    if(withquot)
    {
        nn_printer_writechar(pr, '"');
    }
    return true;
}

bool nn_printer_vwritefmttostring(NNPrinter* pr, const char* fmt, va_list va)
{
    #if 0
        size_t wsz;
        size_t needed;
        char* buf;
        va_list copy;
        va_copy(copy, va);
        needed = 1 + vsnprintf(NULL, 0, fmt, copy);
        va_end(copy);
        buf = (char*)nn_gcmem_allocate(pr->pvm, sizeof(char), needed + 1);
        if(!buf)
        {
            return false;
        }
        memset(buf, 0, needed + 1);
        wsz = vsnprintf(buf, needed, fmt, va);
        nn_printer_writestringl(pr, buf, wsz);
        nn_util_memfree(pr->pvm, buf);
    #else
        dyn_strbuf_appendformatv(pr->strbuf, fmt, va);
    #endif
    return true;
}

bool nn_printer_vwritefmt(NNPrinter* pr, const char* fmt, va_list va)
{
    if(pr->wrmode == NEON_PRMODE_STRING)
    {
        return nn_printer_vwritefmttostring(pr, fmt, va);
    }
    else if(pr->wrmode == NEON_PRMODE_FILE)
    {
        vfprintf(pr->handle, fmt, va);
        if(pr->shouldflush)
        {
            fflush(pr->handle);
        }
    }
    return true;
}

bool nn_printer_writefmt(NNPrinter* pr, const char* fmt, ...) NEON_ATTR_PRINTFLIKE(2, 3);

bool nn_printer_writefmt(NNPrinter* pr, const char* fmt, ...)
{
    bool b;
    va_list va;
    va_start(va, fmt);
    b = nn_printer_vwritefmt(pr, fmt, va);
    va_end(va);
    return b;
}

void nn_printer_printfunction(NNPrinter* pr, NNObjFuncScript* func)
{
    if(func->name == NULL)
    {
        nn_printer_writefmt(pr, "<script at %p>", (void*)func);
    }
    else
    {
        if(func->isvariadic)
        {
            nn_printer_writefmt(pr, "<function %s(%d...) at %p>", func->name->sbuf->data, func->arity, (void*)func);
        }
        else
        {
            nn_printer_writefmt(pr, "<function %s(%d) at %p>", func->name->sbuf->data, func->arity, (void*)func);
        }
    }
}

void nn_printer_printarray(NNPrinter* pr, NNObjArray* list)
{
    size_t i;
    size_t vsz;
    bool isrecur;
    NNValue val;
    NNObjArray* subarr;
    vsz = list->varray->listcount;
    nn_printer_writefmt(pr, "[");
    for(i = 0; i < vsz; i++)
    {
        isrecur = false;
        val = list->varray->listitems[i];
        if(nn_value_isarray(val))
        {
            subarr = nn_value_asarray(val);
            if(subarr == list)
            {
                isrecur = true;
            }
        }
        if(isrecur)
        {
            nn_printer_writefmt(pr, "<recursion>");
        }
        else
        {
            nn_printer_printvalue(pr, val, true, true);
        }
        if(i != vsz - 1)
        {
            nn_printer_writefmt(pr, ", ");
        }
        if(pr->shortenvalues && (i >= pr->maxvallength))
        {
            nn_printer_writefmt(pr, " [%ld items]", vsz);
            break;
        }
    }
    nn_printer_writefmt(pr, "]");
}

void nn_printer_printdict(NNPrinter* pr, NNObjDict* dict)
{
    size_t i;
    size_t dsz;
    bool keyisrecur;
    bool valisrecur;
    NNValue val;
    NNObjDict* subdict;
    NNProperty* field;
    dsz = dict->names->listcount;
    nn_printer_writefmt(pr, "{");
    for(i = 0; i < dsz; i++)
    {
        valisrecur = false;
        keyisrecur = false;
        val = dict->names->listitems[i];
        if(nn_value_isdict(val))
        {
            subdict = nn_value_asdict(val);
            if(subdict == dict)
            {
                valisrecur = true;
            }
        }
        if(valisrecur)
        {
            nn_printer_writefmt(pr, "<recursion>");
        }
        else
        {
            nn_printer_printvalue(pr, val, true, true);
        }
        nn_printer_writefmt(pr, ": ");
        field = nn_table_getfield(dict->htab, dict->names->listitems[i]);
        if(field != NULL)
        {
            if(nn_value_isdict(field->value))
            {
                subdict = nn_value_asdict(field->value);
                if(subdict == dict)
                {
                    keyisrecur = true;
                }
            }
            if(keyisrecur)
            {
                nn_printer_writefmt(pr, "<recursion>");
            }
            else
            {
                nn_printer_printvalue(pr, field->value, true, true);
            }
        }
        if(i != dsz - 1)
        {
            nn_printer_writefmt(pr, ", ");
        }
        if(pr->shortenvalues && (pr->maxvallength >= i))
        {
            nn_printer_writefmt(pr, " [%ld items]", dsz);
            break;
        }
    }
    nn_printer_writefmt(pr, "}");
}

void nn_printer_printfile(NNPrinter* pr, NNObjFile* file)
{
    nn_printer_writefmt(pr, "<file at %s in mode %s>", file->path->sbuf->data, file->mode->sbuf->data);
}

void nn_printer_printinstance(NNPrinter* pr, NNObjInstance* instance, bool invmethod)
{
    (void)invmethod;
    #if 0
    int arity;
    NNPrinter subw;
    NNValue resv;
    NNValue thisval;
    NNProperty* field;
    NNState* state;
    NNObjString* os;
    NNObjArray* args;
    state = pr->pvm;
    if(invmethod)
    {
        field = nn_table_getfieldbycstr(instance->klass->methods, "toString");
        if(field != NULL)
        {
            args = nn_object_makearray(state);
            thisval = nn_value_fromobject(instance);
            arity = nn_nestcall_prepare(state, field->value, thisval, args);
            fprintf(stderr, "arity = %d\n", arity);
            nn_vm_stackpop(state);
            nn_vm_stackpush(state, thisval);
            if(nn_nestcall_callfunction(state, field->value, thisval, args, &resv))
            {
                nn_printer_makestackstring(state, &subw);
                nn_printer_printvalue(&subw, resv, false, false);
                os = nn_printer_takestring(&subw);
                nn_printer_writestringl(pr, os->sbuf->data, os->sbuf->length);
                #if 0
                    nn_vm_stackpop(state);
                #endif
                return;
            }
        }
    }
    #endif
    nn_printer_writefmt(pr, "<instance of %s at %p>", instance->klass->name->sbuf->data, (void*)instance);
}

void nn_printer_printobject(NNPrinter* pr, NNValue value, bool fixstring, bool invmethod)
{
    NNObject* obj;
    obj = nn_value_asobject(value);
    switch(obj->type)
    {
        case NEON_OBJTYPE_SWITCH:
            {
                nn_printer_writestring(pr, "<switch>");
            }
            break;
        case NEON_OBJTYPE_USERDATA:
            {
                nn_printer_writefmt(pr, "<userdata %s>", nn_value_asuserdata(value)->name);
            }
            break;
        case NEON_OBJTYPE_RANGE:
            {
                NNObjRange* range;
                range = nn_value_asrange(value);
                nn_printer_writefmt(pr, "<range %d .. %d>", range->lower, range->upper);
            }
            break;
        case NEON_OBJTYPE_FILE:
            {
                nn_printer_printfile(pr, nn_value_asfile(value));
            }
            break;
        case NEON_OBJTYPE_DICT:
            {
                nn_printer_printdict(pr, nn_value_asdict(value));
            }
            break;
        case NEON_OBJTYPE_ARRAY:
            {
                nn_printer_printarray(pr, nn_value_asarray(value));
            }
            break;
        case NEON_OBJTYPE_FUNCBOUND:
            {
                NNObjFuncBound* bn;
                bn = nn_value_asfuncbound(value);
                nn_printer_printfunction(pr, bn->method->scriptfunc);
            }
            break;
        case NEON_OBJTYPE_MODULE:
            {
                NNObjModule* mod;
                mod = nn_value_asmodule(value);
                nn_printer_writefmt(pr, "<module '%s' at '%s'>", mod->name->sbuf->data, mod->physicalpath->sbuf->data);
            }
            break;
        case NEON_OBJTYPE_CLASS:
            {
                NNObjClass* klass;
                klass = nn_value_asclass(value);
                nn_printer_writefmt(pr, "<class %s at %p>", klass->name->sbuf->data, (void*)klass);
            }
            break;
        case NEON_OBJTYPE_FUNCCLOSURE:
            {
                NNObjFuncClosure* cls;
                cls = nn_value_asfuncclosure(value);
                nn_printer_printfunction(pr, cls->scriptfunc);
            }
            break;
        case NEON_OBJTYPE_FUNCSCRIPT:
            {
                NNObjFuncScript* fn;
                fn = nn_value_asfuncscript(value);
                nn_printer_printfunction(pr, fn);
            }
            break;
        case NEON_OBJTYPE_INSTANCE:
            {
                /* @TODO: support the toString() override */
                NNObjInstance* instance;
                instance = nn_value_asinstance(value);
                nn_printer_printinstance(pr, instance, invmethod);
            }
            break;
        case NEON_OBJTYPE_FUNCNATIVE:
            {
                NNObjFuncNative* native;
                native = nn_value_asfuncnative(value);
                nn_printer_writefmt(pr, "<function %s(native) at %p>", native->name, (void*)native);
            }
            break;
        case NEON_OBJTYPE_UPVALUE:
            {
                nn_printer_writefmt(pr, "<upvalue>");
            }
            break;
        case NEON_OBJTYPE_STRING:
            {
                NNObjString* string;
                string = nn_value_asstring(value);
                if(fixstring)
                {
                    nn_printer_writequotedstring(pr, string->sbuf->data, string->sbuf->length, true);
                }
                else
                {
                    nn_printer_writestringl(pr, string->sbuf->data, string->sbuf->length);
                }
            }
            break;
    }
}

void nn_printer_printvalue(NNPrinter* pr, NNValue value, bool fixstring, bool invmethod)
{
    switch(value.type)
    {
        case NEON_VALTYPE_EMPTY:
            {
                nn_printer_writestring(pr, "<empty>");
            }
            break;
        case NEON_VALTYPE_NULL:
            {
                nn_printer_writestring(pr, "null");
            }
            break;
        case NEON_VALTYPE_BOOL:
            {
                nn_printer_writestring(pr, nn_value_asbool(value) ? "true" : "false");
            }
            break;
        case NEON_VALTYPE_NUMBER:
            {
                nn_printer_writefmt(pr, "%.16g", nn_value_asnumber(value));
            }
            break;
        case NEON_VALTYPE_OBJ:
            {
                nn_printer_printobject(pr, value, fixstring, invmethod);
            }
            break;
        default:
            break;
    }
}

NNObjString* nn_value_tostring(NNState* state, NNValue value)
{
    NNPrinter pr;
    NNObjString* s;
    nn_printer_makestackstring(state, &pr);
    nn_printer_printvalue(&pr, value, false, true);
    s = nn_printer_takestring(&pr);
    return s;
}

const char* nn_value_objecttypename(NNObject* object)
{
    switch(object->type)
    {
        case NEON_OBJTYPE_MODULE:
            return "module";
        case NEON_OBJTYPE_RANGE:
            return "range";
        case NEON_OBJTYPE_FILE:
            return "file";
        case NEON_OBJTYPE_DICT:
            return "dictionary";
        case NEON_OBJTYPE_ARRAY:
            return "array";
        case NEON_OBJTYPE_CLASS:
            return "class";
        case NEON_OBJTYPE_FUNCSCRIPT:
        case NEON_OBJTYPE_FUNCNATIVE:
        case NEON_OBJTYPE_FUNCCLOSURE:
        case NEON_OBJTYPE_FUNCBOUND:
            return "function";
        case NEON_OBJTYPE_INSTANCE:
            return ((NNObjInstance*)object)->klass->name->sbuf->data;
        case NEON_OBJTYPE_STRING:
            return "string";
        case NEON_OBJTYPE_USERDATA:
            return "userdata";
        case NEON_OBJTYPE_SWITCH:
            return "switch";
        default:
            break;
    }
    return "unknown";
}

const char* nn_value_typename(NNValue value)
{
    if(nn_value_isempty(value))
    {
        return "empty";
    }
    if(nn_value_isnull(value))
    {
        return "null";
    }
    else if(nn_value_isbool(value))
    {
        return "boolean";
    }
    else if(nn_value_isnumber(value))
    {
        return "number";
    }
    else if(nn_value_isobject(value))
    {
        return nn_value_objecttypename(nn_value_asobject(value));
    }
    return "unknown";
}

bool nn_value_compobject(NNState* state, NNValue a, NNValue b)
{
    size_t i;
    NNObjType ta;
    NNObjType tb;
    NNObject* oa;
    NNObject* ob;
    NNObjString* stra;
    NNObjString* strb;
    NNObjArray* arra;
    NNObjArray* arrb;
    oa = nn_value_asobject(a);
    ob = nn_value_asobject(b);
    ta = oa->type;
    tb = ob->type;
    if(ta == tb)
    {
        if(ta == NEON_OBJTYPE_STRING)
        {
            stra = (NNObjString*)oa;
            strb = (NNObjString*)ob;
            if(stra->sbuf->length == strb->sbuf->length)
            {
                if(memcmp(stra->sbuf->data, strb->sbuf->data, stra->sbuf->length) == 0)
                {
                    return true;
                }
                return false;
            }
        }
        if(ta == NEON_OBJTYPE_ARRAY)
        {
            arra = (NNObjArray*)oa;
            arrb = (NNObjArray*)ob;
            if(arra->varray->listcount == arrb->varray->listcount)
            {
                for(i=0; i<(size_t)arra->varray->listcount; i++)
                {
                    if(!nn_value_compare(state, arra->varray->listitems[i], arrb->varray->listitems[i]))
                    {
                        return false;
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool nn_value_compare_actual(NNState* state, NNValue a, NNValue b)
{
    if(a.type != b.type)
    {
        return false;
    }
    switch(a.type)
    {
        case NEON_VALTYPE_NULL:
        case NEON_VALTYPE_EMPTY:
            {
                return true;
            }
            break;
        case NEON_VALTYPE_BOOL:
            {
                return nn_value_asbool(a) == nn_value_asbool(b);
            }
            break;
        case NEON_VALTYPE_NUMBER:
            {
                return (nn_value_asnumber(a) == nn_value_asnumber(b));
            }
            break;
        case NEON_VALTYPE_OBJ:
            {
                if(nn_value_asobject(a) == nn_value_asobject(b))
                {
                    return true;
                }
                return nn_value_compobject(state, a, b);
            }
            break;
        default:
            break;
    }
    return false;
}


bool nn_value_compare(NNState* state, NNValue a, NNValue b)
{
    bool r;
    r = nn_value_compare_actual(state, a, b);
    return r;
}

uint32_t nn_util_hashbits(uint64_t hash)
{
    /*
    // From v8's ComputeLongHash() which in turn cites:
    // Thomas Wang, Integer Hash Functions.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    // hash = (hash << 18) - hash - 1;
    */
    hash = ~hash + (hash << 18);
    hash = hash ^ (hash >> 31);
    /* hash = (hash + (hash << 2)) + (hash << 4); */
    hash = hash * 21;
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t)(hash & 0x3fffffff);
}

uint32_t nn_util_hashdouble(double value)
{
    NNDoubleHashUnion bits;
    bits.num = value;
    return nn_util_hashbits(bits.bits);
}

uint32_t nn_util_hashstring(const char* key, int length)
{
    uint32_t hash;
    const char* be;
    hash = 2166136261u;
    be = key + length;
    while(key < be)
    {
        hash = (hash ^ *key++) * 16777619;
    }
    return hash;
    /* return siphash24(127, 255, key, length); */
}

uint32_t nn_object_hashobject(NNObject* object)
{
    switch(object->type)
    {
        case NEON_OBJTYPE_CLASS:
            {
                /* Classes just use their name. */
                return ((NNObjClass*)object)->name->hash;
            }
            break;
        case NEON_OBJTYPE_FUNCSCRIPT:
            {
                /*
                // Allow bare (non-closure) functions so that we can use a map to find
                // existing constants in a function's constant table. This is only used
                // internally. Since user code never sees a non-closure function, they
                // cannot use them as map keys.
                */
                NNObjFuncScript* fn;
                fn = (NNObjFuncScript*)object;
                return nn_util_hashdouble(fn->arity) ^ nn_util_hashdouble(fn->blob.count);
            }
            break;
        case NEON_OBJTYPE_STRING:
            {
                return ((NNObjString*)object)->hash;
            }
            break;
        default:
            break;
    }
    return 0;
}

uint32_t nn_value_hashvalue(NNValue value)
{
    switch(value.type)
    {
        case NEON_VALTYPE_BOOL:
            return nn_value_asbool(value) ? 3 : 5;
        case NEON_VALTYPE_NULL:
            return 7;
        case NEON_VALTYPE_NUMBER:
            return nn_util_hashdouble(nn_value_asnumber(value));
        case NEON_VALTYPE_OBJ:
            return nn_object_hashobject(nn_value_asobject(value));
        default:
            /* NEON_VALTYPE_EMPTY */
            break;
    }
    return 0;
}


/**
 * returns the greater of the two values.
 * this function encapsulates the object hierarchy
 */
NNValue nn_value_findgreater(NNValue a, NNValue b)
{
    NNObjString* osa;
    NNObjString* osb;    
    if(nn_value_isnull(a))
    {
        return b;
    }
    else if(nn_value_isbool(a))
    {
        if(nn_value_isnull(b) || (nn_value_isbool(b) && nn_value_asbool(b) == false))
        {
            /* only null, false and false are lower than numbers */
            return a;
        }
        else
        {
            return b;
        }
    }
    else if(nn_value_isnumber(a))
    {
        if(nn_value_isnull(b) || nn_value_isbool(b))
        {
            return a;
        }
        else if(nn_value_isnumber(b))
        {
            if(nn_value_asnumber(a) >= nn_value_asnumber(b))
            {
                return a;
            }
            return b;
        }
        else
        {
            /* every other thing is greater than a number */
            return b;
        }
    }
    else if(nn_value_isobject(a))
    {
        if(nn_value_isstring(a) && nn_value_isstring(b))
        {
            osa = nn_value_asstring(a);
            osb = nn_value_asstring(b);
            if(strncmp(osa->sbuf->data, osb->sbuf->data, osa->sbuf->length) >= 0)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isfuncscript(a) && nn_value_isfuncscript(b))
        {
            if(nn_value_asfuncscript(a)->arity >= nn_value_asfuncscript(b)->arity)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isfuncclosure(a) && nn_value_isfuncclosure(b))
        {
            if(nn_value_asfuncclosure(a)->scriptfunc->arity >= nn_value_asfuncclosure(b)->scriptfunc->arity)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isrange(a) && nn_value_isrange(b))
        {
            if(nn_value_asrange(a)->lower >= nn_value_asrange(b)->lower)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isclass(a) && nn_value_isclass(b))
        {
            if(nn_value_asclass(a)->methods->count >= nn_value_asclass(b)->methods->count)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isarray(a) && nn_value_isarray(b))
        {
            if(nn_value_asarray(a)->varray->listcount >= nn_value_asarray(b)->varray->listcount)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isdict(a) && nn_value_isdict(b))
        {
            if(nn_value_asdict(a)->names->listcount >= nn_value_asdict(b)->names->listcount)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isfile(a) && nn_value_isfile(b))
        {
            if(strcmp(nn_value_asfile(a)->path->sbuf->data, nn_value_asfile(b)->path->sbuf->data) >= 0)
            {
                return a;
            }
            return b;
        }
        else if(nn_value_isobject(b))
        {
            if(nn_value_asobject(a)->type >= nn_value_asobject(b)->type)
            {
                return a;
            }
            return b;
        }
        else
        {
            return a;
        }
    }
    return a;
}

/**
 * sorts values in an array using the bubble-sort algorithm
 */
void nn_value_sortvalues(NNState* state, NNValue* values, int count)
{
    int i;
    int j;
    NNValue temp;
    for(i = 0; i < count; i++)
    {
        for(j = 0; j < count; j++)
        {
            if(nn_value_compare(state, values[j], nn_value_findgreater(values[i], values[j])))
            {
                temp = values[i];
                values[i] = values[j];
                values[j] = temp;
                if(nn_value_isarray(values[i]))
                {
                    nn_value_sortvalues(state, nn_value_asarray(values[i])->varray->listitems, nn_value_asarray(values[i])->varray->listcount);
                }

                if(nn_value_isarray(values[j]))
                {
                    nn_value_sortvalues(state, nn_value_asarray(values[j])->varray->listitems, nn_value_asarray(values[j])->varray->listcount);
                }
            }
        }
    }
}

NNValue nn_value_copyvalue(NNState* state, NNValue value)
{
    if(nn_value_isobject(value))
    {
        switch(nn_value_asobject(value)->type)
        {
            case NEON_OBJTYPE_STRING:
                {
                    NNObjString* string;
                    string = nn_value_asstring(value);
                    return nn_value_fromobject(nn_string_copylen(state, string->sbuf->data, string->sbuf->length));
                }
                break;
            case NEON_OBJTYPE_ARRAY:
            {
                size_t i;
                NNObjArray* list;
                NNObjArray* newlist;
                list = nn_value_asarray(value);
                newlist = nn_object_makearray(state);
                nn_vm_stackpush(state, nn_value_fromobject(newlist));
                for(i = 0; i < list->varray->listcount; i++)
                {
                    nn_vallist_push(newlist->varray, list->varray->listitems[i]);
                }
                nn_vm_stackpop(state);
                return nn_value_fromobject(newlist);
            }
            /*
            case NEON_OBJTYPE_DICT:
                {
                    NNObjDict *dict;
                    NNObjDict *newdict;
                    dict = nn_value_asdict(value);
                    newdict = nn_object_makedict(state);
                    // @TODO: Figure out how to handle dictionary values correctly
                    // remember that copying keys is redundant and unnecessary
                }
                break;
            */
            default:
                break;
        }
    }
    return value;
}

NNObject* nn_object_allocobject(NNState* state, size_t size, NNObjType type)
{
    NNObject* object;
    object = (NNObject*)nn_gcmem_allocate(state, size, 1);
    object->type = type;
    object->mark = !state->markvalue;
    object->stale = false;
    object->pvm = state;
    object->next = state->vmstate.linkedobjects;
    state->vmstate.linkedobjects = object;
    #if defined(DEBUG_GC) && DEBUG_GC
    nn_printer_writefmt(state->debugwriter, "%p allocate %ld for %d\n", (void*)object, size, type);
    #endif
    return object;
}

NNObjUserdata* nn_object_makeuserdata(NNState* state, void* pointer, const char* name)
{
    NNObjUserdata* ptr;
    ptr = (NNObjUserdata*)nn_object_allocobject(state, sizeof(NNObjUserdata), NEON_OBJTYPE_USERDATA);
    ptr->pointer = pointer;
    ptr->name = nn_util_strdup(state, name);
    ptr->ondestroyfn = NULL;
    return ptr;
}

NNObjModule* nn_module_make(NNState* state, const char* name, const char* file, bool imported)
{
    NNObjModule* module;
    module = (NNObjModule*)nn_object_allocobject(state, sizeof(NNObjModule), NEON_OBJTYPE_MODULE);
    module->deftable = nn_table_make(state);
    module->name = nn_string_copycstr(state, name);
    module->physicalpath = nn_string_copycstr(state, file);
    module->unloader = NULL;
    module->preloader = NULL;
    module->handle = NULL;
    module->imported = imported;
    return module;
}

void nn_module_destroy(NNState* state, NNObjModule* module)
{
    nn_table_destroy(module->deftable);
    /*
    nn_util_memfree(state, module->name);
    nn_util_memfree(state, module->physicalpath);
    */
    if(module->unloader != NULL && module->imported)
    {
        ((NNModLoaderFN)module->unloader)(state);
    }
    if(module->handle != NULL)
    {
        nn_import_closemodule(module->handle);
    }
}

void nn_module_setfilefield(NNState* state, NNObjModule* module)
{
    return;
    nn_table_setcstr(module->deftable, "__file__", nn_value_fromobject(nn_string_copyobjstr(state, module->physicalpath)));
}

NNObjSwitch* nn_object_makeswitch(NNState* state)
{
    NNObjSwitch* sw;
    sw = (NNObjSwitch*)nn_object_allocobject(state, sizeof(NNObjSwitch), NEON_OBJTYPE_SWITCH);
    sw->table = nn_table_make(state);
    sw->defaultjump = -1;
    sw->exitjump = -1;
    return sw;
}

NNObjArray* nn_object_makearray(NNState* state)
{
    return nn_array_make(state);
}

NNObjRange* nn_object_makerange(NNState* state, int lower, int upper)
{
    NNObjRange* range;
    range = (NNObjRange*)nn_object_allocobject(state, sizeof(NNObjRange), NEON_OBJTYPE_RANGE);
    range->lower = lower;
    range->upper = upper;
    if(upper > lower)
    {
        range->range = upper - lower;
    }
    else
    {
        range->range = lower - upper;
    }
    return range;
}

NNObjDict* nn_object_makedict(NNState* state)
{
    NNObjDict* dict;
    dict = (NNObjDict*)nn_object_allocobject(state, sizeof(NNObjDict), NEON_OBJTYPE_DICT);
    dict->names = nn_vallist_make(state);
    dict->htab = nn_table_make(state);
    return dict;
}

NNObjFile* nn_object_makefile(NNState* state, FILE* handle, bool isstd, const char* path, const char* mode)
{
    NNObjFile* file;
    file = (NNObjFile*)nn_object_allocobject(state, sizeof(NNObjFile), NEON_OBJTYPE_FILE);
    file->isopen = false;
    file->mode = nn_string_copycstr(state, mode);
    file->path = nn_string_copycstr(state, path);
    file->isstd = isstd;
    file->handle = handle;
    file->istty = false;
    file->number = -1;
    if(file->handle != NULL)
    {
        file->isopen = true;
    }
    return file;
}

void nn_file_destroy(NNState* state, NNObjFile* file)
{
    nn_fileobject_close(file);
    nn_gcmem_release(state, file, sizeof(NNObjFile));
}

void nn_file_mark(NNState* state, NNObjFile* file)
{
    nn_gcmem_markobject(state, (NNObject*)file->mode);
    nn_gcmem_markobject(state, (NNObject*)file->path);
}

NNObjFuncBound* nn_object_makefuncbound(NNState* state, NNValue receiver, NNObjFuncClosure* method)
{
    NNObjFuncBound* bound;
    bound = (NNObjFuncBound*)nn_object_allocobject(state, sizeof(NNObjFuncBound), NEON_OBJTYPE_FUNCBOUND);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

NNObjClass* nn_object_makeclass(NNState* state, NNObjString* name)
{
    NNObjClass* klass;
    klass = (NNObjClass*)nn_object_allocobject(state, sizeof(NNObjClass), NEON_OBJTYPE_CLASS);
    klass->name = name;
    klass->instprops = nn_table_make(state);
    klass->staticproperties = nn_table_make(state);
    klass->methods = nn_table_make(state);
    klass->staticmethods = nn_table_make(state);
    klass->constructor = nn_value_makeempty();
    klass->destructor = nn_value_makeempty();
    klass->superclass = NULL;
    return klass;
}

void nn_class_destroy(NNState* state, NNObjClass* klass)
{
    nn_table_destroy(klass->methods);
    nn_table_destroy(klass->staticmethods);
    nn_table_destroy(klass->instprops);
    nn_table_destroy(klass->staticproperties);
    /*
    // We are not freeing the initializer because it's a closure and will still be freed accordingly later.
    */
    memset(klass, 0, sizeof(NNObjClass));
    nn_gcmem_release(state, klass, sizeof(NNObjClass));   
}

bool nn_class_inheritfrom(NNObjClass* subclass, NNObjClass* superclass)
{
    nn_table_addall(superclass->instprops, subclass->instprops);
    nn_table_addall(superclass->methods, subclass->methods);
    subclass->superclass = superclass;
    return true;
}

bool nn_class_defproperty(NNObjClass* klass, const char* cstrname, NNValue val)
{
    return nn_table_setcstr(klass->instprops, cstrname, val);
}

bool nn_class_defcallablefieldptr(NNState* state, NNObjClass* klass, const char* cstrname, NNNativeFN function, void* uptr)
{
    NNObjString* oname;
    NNObjFuncNative* ofn;
    oname = nn_string_copycstr(state, cstrname);
    ofn = nn_object_makefuncnative(state, function, cstrname, uptr);
    return nn_table_setwithtype(klass->instprops, nn_value_fromobject(oname), nn_value_fromobject(ofn), NEON_PROPTYPE_FUNCTION, true);
}

bool nn_class_defcallablefield(NNState* state, NNObjClass* klass, const char* cstrname, NNNativeFN function)
{
    return nn_class_defcallablefieldptr(state, klass, cstrname, function, NULL);
}

bool nn_class_setstaticpropertycstr(NNObjClass* klass, const char* cstrname, NNValue val)
{
    return nn_table_setcstr(klass->staticproperties, cstrname, val);
}

bool nn_class_setstaticproperty(NNObjClass* klass, NNObjString* name, NNValue val)
{
    return nn_class_setstaticpropertycstr(klass, name->sbuf->data, val);
}

bool nn_class_defnativeconstructorptr(NNState* state, NNObjClass* klass, NNNativeFN function, void* uptr)
{
    const char* cname;
    NNObjFuncNative* ofn;
    cname = "constructor";
    ofn = nn_object_makefuncnative(state, function, cname, uptr);
    klass->constructor = nn_value_fromobject(ofn);
    return true;
}

bool nn_class_defnativeconstructor(NNState* state, NNObjClass* klass, NNNativeFN function)
{
    return nn_class_defnativeconstructorptr(state, klass, function, NULL);
}

bool nn_class_defmethod(NNState* state, NNObjClass* klass, const char* name, NNValue val)
{
    (void)state;
    return nn_table_setcstr(klass->methods, name, val);
}

bool nn_class_defnativemethodptr(NNState* state, NNObjClass* klass, const char* name, NNNativeFN function, void* ptr)
{
    NNObjFuncNative* ofn;
    ofn = nn_object_makefuncnative(state, function, name, ptr);
    return nn_class_defmethod(state, klass, name, nn_value_fromobject(ofn));
}

bool nn_class_defnativemethod(NNState* state, NNObjClass* klass, const char* name, NNNativeFN function)
{
    return nn_class_defnativemethodptr(state, klass, name, function, NULL);
}

bool nn_class_defstaticnativemethodptr(NNState* state, NNObjClass* klass, const char* name, NNNativeFN function, void* uptr)
{
    NNObjFuncNative* ofn;
    ofn = nn_object_makefuncnative(state, function, name, uptr);
    return nn_table_setcstr(klass->staticmethods, name, nn_value_fromobject(ofn));
}

bool nn_class_defstaticnativemethod(NNState* state, NNObjClass* klass, const char* name, NNNativeFN function)
{
    return nn_class_defstaticnativemethodptr(state, klass, name, function, NULL);
}

NNProperty* nn_class_getmethodfield(NNObjClass* klass, NNObjString* name)
{
    NNProperty* field;
    field = nn_table_getfield(klass->methods, nn_value_fromobject(name));
    if(field != NULL)
    {
        return field;
    }
    if(klass->superclass != NULL)
    {
        return nn_class_getmethodfield(klass->superclass, name);
    }
    return NULL;
}

NNProperty* nn_class_getpropertyfield(NNObjClass* klass, NNObjString* name)
{
    NNProperty* field;
    field = nn_table_getfield(klass->instprops, nn_value_fromobject(name));
    return field;
}

NNProperty* nn_class_getstaticproperty(NNObjClass* klass, NNObjString* name)
{
    return nn_table_getfieldbyostr(klass->staticproperties, name);
}

NNProperty* nn_class_getstaticmethodfield(NNObjClass* klass, NNObjString* name)
{
    NNProperty* field;
    field = nn_table_getfield(klass->staticmethods, nn_value_fromobject(name));
    return field;
}

NNObjInstance* nn_object_makeinstance(NNState* state, NNObjClass* klass)
{
    NNObjInstance* instance;
    instance = (NNObjInstance*)nn_object_allocobject(state, sizeof(NNObjInstance), NEON_OBJTYPE_INSTANCE);
    /* gc fix */
    nn_vm_stackpush(state, nn_value_fromobject(instance));
    instance->active = true;
    instance->klass = klass;
    instance->properties = nn_table_make(state);
    if(klass->instprops->count > 0)
    {
        nn_table_copy(klass->instprops, instance->properties);
    }
    /* gc fix */
    nn_vm_stackpop(state);
    return instance;
}

void nn_instance_mark(NNState* state, NNObjInstance* instance)
{
    if(instance->active == false)
    {
        nn_state_warn(state, "trying to mark inactive instance <%p>!", instance);
        return;
    }
    nn_gcmem_markobject(state, (NNObject*)instance->klass);
    nn_table_mark(state, instance->properties);
}

void nn_instance_destroy(NNState* state, NNObjInstance* instance)
{
    if(!nn_value_isempty(instance->klass->destructor))
    {
        if(!nn_vm_callvaluewithobject(state, instance->klass->constructor, nn_value_fromobject(instance), 0))
        {
            
        }
    }
    nn_table_destroy(instance->properties);
    instance->properties = NULL;
    instance->active = false;
    nn_gcmem_release(state, instance, sizeof(NNObjInstance));
}

bool nn_instance_defproperty(NNObjInstance* instance, const char *cstrname, NNValue val)
{
    return nn_table_setcstr(instance->properties, cstrname, val);
}

NNObjFuncScript* nn_object_makefuncscript(NNState* state, NNObjModule* module, NNFuncType type)
{
    NNObjFuncScript* function;
    function = (NNObjFuncScript*)nn_object_allocobject(state, sizeof(NNObjFuncScript), NEON_OBJTYPE_FUNCSCRIPT);
    function->arity = 0;
    function->upvalcount = 0;
    function->isvariadic = false;
    function->name = NULL;
    function->type = type;
    function->module = module;
    nn_blob_init(state, &function->blob);
    return function;
}

void nn_funcscript_destroy(NNState* state, NNObjFuncScript* function)
{
    nn_blob_destroy(state, &function->blob);
}

NNObjFuncNative* nn_object_makefuncnative(NNState* state, NNNativeFN function, const char* name, void* uptr)
{
    NNObjFuncNative* native;
    native = (NNObjFuncNative*)nn_object_allocobject(state, sizeof(NNObjFuncNative), NEON_OBJTYPE_FUNCNATIVE);
    native->natfunc = function;
    native->name = name;
    native->type = NEON_FUNCTYPE_FUNCTION;
    native->userptr = uptr;
    return native;
}

NNObjFuncClosure* nn_object_makefuncclosure(NNState* state, NNObjFuncScript* function)
{
    int i;
    NNObjUpvalue** upvals;
    NNObjFuncClosure* closure;
    upvals = (NNObjUpvalue**)nn_gcmem_allocate(state, sizeof(NNObjUpvalue*), function->upvalcount);
    for(i = 0; i < function->upvalcount; i++)
    {
        upvals[i] = NULL;
    }
    closure = (NNObjFuncClosure*)nn_object_allocobject(state, sizeof(NNObjFuncClosure), NEON_OBJTYPE_FUNCCLOSURE);
    closure->scriptfunc = function;
    closure->upvalues = upvals;
    closure->upvalcount = function->upvalcount;
    return closure;
}

NNObjString* nn_string_makefromstrbuf(NNState* state, StringBuffer* sbuf, uint32_t hash)
{
    NNObjString* rs;
    rs = (NNObjString*)nn_object_allocobject(state, sizeof(NNObjString), NEON_OBJTYPE_STRING);
    rs->sbuf = sbuf;
    rs->hash = hash;
    nn_vm_stackpush(state, nn_value_fromobject(rs));
    nn_table_set(state->strings, nn_value_fromobject(rs), nn_value_makenull());
    nn_vm_stackpop(state);
    return rs;
}

NNObjString* nn_string_allocstring(NNState* state, const char* estr, size_t elen, uint32_t hash, bool istaking)
{
    StringBuffer* sbuf;
    (void)istaking;
    sbuf = dyn_strbuf_makeempty(elen);
    dyn_strbuf_appendstrn(sbuf, estr, elen);
    return nn_string_makefromstrbuf(state, sbuf, hash);
}

/*
NNObjString* nn_string_borrow(NNState* state, const char* estr, size_t elen)
{
    uint32_t hash;
    StringBuffer* sbuf;
    hash = nn_util_hashstring(estr, length);
    sbuf = dyn_strbuf_makeborrowed(estr, elen);
    return nn_string_makefromstrbuf(state, sbuf, hash);
}
*/

size_t nn_string_getlength(NNObjString* os)
{
    return os->sbuf->length;
}

const char* nn_string_getdata(NNObjString* os)
{
    return os->sbuf->data;
}

const char* nn_string_getcstr(NNObjString* os)
{
    return nn_string_getdata(os);
}

void nn_string_destroy(NNState* state, NNObjString* str)
{
    dyn_strbuf_destroy(str->sbuf);
    nn_gcmem_release(state, str, sizeof(NNObjString));
}

NNObjString* nn_string_takelen(NNState* state, char* chars, int length)
{
    uint32_t hash;
    NNObjString* rs;
    hash = nn_util_hashstring(chars, length);
    rs = nn_table_findstring(state->strings, chars, length, hash);
    if(rs == NULL)
    {
        rs = nn_string_allocstring(state, chars, length, hash, true);
    }
    nn_gcmem_freearray(state, sizeof(char), chars, (size_t)length + 1);
    return rs;
}

NNObjString* nn_string_copylen(NNState* state, const char* chars, int length)
{
    uint32_t hash;
    NNObjString* rs;
    hash = nn_util_hashstring(chars, length);
    rs = nn_table_findstring(state->strings, chars, length, hash);
    if(rs != NULL)
    {
        return rs;
    }
    rs = nn_string_allocstring(state, chars, length, hash, false);
    return rs;
}

NNObjString* nn_string_takecstr(NNState* state, char* chars)
{
    return nn_string_takelen(state, chars, strlen(chars));
}

NNObjString* nn_string_copycstr(NNState* state, const char* chars)
{
    return nn_string_copylen(state, chars, strlen(chars));
}

NNObjString* nn_string_copyobjstr(NNState* state, NNObjString* os)
{
    return nn_string_copylen(state, os->sbuf->data, os->sbuf->length);
}

NNObjUpvalue* nn_object_makeupvalue(NNState* state, NNValue* slot, int stackpos)
{
    NNObjUpvalue* upvalue;
    upvalue = (NNObjUpvalue*)nn_object_allocobject(state, sizeof(NNObjUpvalue), NEON_OBJTYPE_UPVALUE);
    upvalue->closed = nn_value_makenull();
    upvalue->location = *slot;
    upvalue->next = NULL;
    upvalue->stackpos = stackpos;
    return upvalue;
}

static const char* g_strthis = "this";
static const char* g_strsuper = "super";

NNAstLexer* nn_astlex_init(NNState* state, const char* source)
{
    NNAstLexer* lex;
    NEON_ASTDEBUG(state, "");
    lex = (NNAstLexer*)nn_gcmem_allocate(state, sizeof(NNAstLexer), 1);
    lex->pvm = state;
    lex->sourceptr = source;
    lex->start = source;
    lex->line = 1;
    lex->tplstringcount = -1;
    return lex;
}

void nn_astlex_destroy(NNState* state, NNAstLexer* lex)
{
    NEON_ASTDEBUG(state, "");
    nn_gcmem_release(state, lex, sizeof(NNAstLexer));
}

bool nn_astlex_isatend(NNAstLexer* lex)
{
    return *lex->sourceptr == '\0';
}

NNAstToken nn_astlex_maketoken(NNAstLexer* lex, NNAstTokType type)
{
    NNAstToken t;
    t.isglobal = false;
    t.type = type;
    t.start = lex->start;
    t.length = (int)(lex->sourceptr - lex->start);
    t.line = lex->line;
    return t;
}

NNAstToken nn_astlex_errortoken(NNAstLexer* lex, const char* fmt, ...)
{
    int length;
    char* buf;
    va_list va;
    NNAstToken t;
    va_start(va, fmt);
    buf = (char*)nn_gcmem_allocate(lex->pvm, sizeof(char), 1024);
    /* TODO: used to be vasprintf. need to check how much to actually allocate! */
    length = vsprintf(buf, fmt, va);
    va_end(va);
    t.type = NEON_ASTTOK_ERROR;
    t.start = buf;
    t.isglobal = false;
    if(buf != NULL)
    {
        t.length = length;
    }
    else
    {
        t.length = 0;
    }
    t.line = lex->line;
    return t;
}

bool nn_astutil_isdigit(char c)
{
    return c >= '0' && c <= '9';
}

bool nn_astutil_isbinary(char c)
{
    return c == '0' || c == '1';
}

bool nn_astutil_isalpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

bool nn_astutil_isoctal(char c)
{
    return c >= '0' && c <= '7';
}

bool nn_astutil_ishexadecimal(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

const char* nn_astutil_toktype2str(int t)
{
    switch(t)
    {
        case NEON_ASTTOK_NEWLINE: return "NEON_ASTTOK_NEWLINE";
        case NEON_ASTTOK_PARENOPEN: return "NEON_ASTTOK_PARENOPEN";
        case NEON_ASTTOK_PARENCLOSE: return "NEON_ASTTOK_PARENCLOSE";
        case NEON_ASTTOK_BRACKETOPEN: return "NEON_ASTTOK_BRACKETOPEN";
        case NEON_ASTTOK_BRACKETCLOSE: return "NEON_ASTTOK_BRACKETCLOSE";
        case NEON_ASTTOK_BRACEOPEN: return "NEON_ASTTOK_BRACEOPEN";
        case NEON_ASTTOK_BRACECLOSE: return "NEON_ASTTOK_BRACECLOSE";
        case NEON_ASTTOK_SEMICOLON: return "NEON_ASTTOK_SEMICOLON";
        case NEON_ASTTOK_COMMA: return "NEON_ASTTOK_COMMA";
        case NEON_ASTTOK_BACKSLASH: return "NEON_ASTTOK_BACKSLASH";
        case NEON_ASTTOK_EXCLMARK: return "NEON_ASTTOK_EXCLMARK";
        case NEON_ASTTOK_NOTEQUAL: return "NEON_ASTTOK_NOTEQUAL";
        case NEON_ASTTOK_COLON: return "NEON_ASTTOK_COLON";
        case NEON_ASTTOK_AT: return "NEON_ASTTOK_AT";
        case NEON_ASTTOK_DOT: return "NEON_ASTTOK_DOT";
        case NEON_ASTTOK_DOUBLEDOT: return "NEON_ASTTOK_DOUBLEDOT";
        case NEON_ASTTOK_TRIPLEDOT: return "NEON_ASTTOK_TRIPLEDOT";
        case NEON_ASTTOK_PLUS: return "NEON_ASTTOK_PLUS";
        case NEON_ASTTOK_PLUSASSIGN: return "NEON_ASTTOK_PLUSASSIGN";
        case NEON_ASTTOK_INCREMENT: return "NEON_ASTTOK_INCREMENT";
        case NEON_ASTTOK_MINUS: return "NEON_ASTTOK_MINUS";
        case NEON_ASTTOK_MINUSASSIGN: return "NEON_ASTTOK_MINUSASSIGN";
        case NEON_ASTTOK_DECREMENT: return "NEON_ASTTOK_DECREMENT";
        case NEON_ASTTOK_MULTIPLY: return "NEON_ASTTOK_MULTIPLY";
        case NEON_ASTTOK_MULTASSIGN: return "NEON_ASTTOK_MULTASSIGN";
        case NEON_ASTTOK_POWEROF: return "NEON_ASTTOK_POWEROF";
        case NEON_ASTTOK_POWASSIGN: return "NEON_ASTTOK_POWASSIGN";
        case NEON_ASTTOK_DIVIDE: return "NEON_ASTTOK_DIVIDE";
        case NEON_ASTTOK_DIVASSIGN: return "NEON_ASTTOK_DIVASSIGN";
        case NEON_ASTTOK_FLOOR: return "NEON_ASTTOK_FLOOR";
        case NEON_ASTTOK_ASSIGN: return "NEON_ASTTOK_ASSIGN";
        case NEON_ASTTOK_EQUAL: return "NEON_ASTTOK_EQUAL";
        case NEON_ASTTOK_LESSTHAN: return "NEON_ASTTOK_LESSTHAN";
        case NEON_ASTTOK_LESSEQUAL: return "NEON_ASTTOK_LESSEQUAL";
        case NEON_ASTTOK_LEFTSHIFT: return "NEON_ASTTOK_LEFTSHIFT";
        case NEON_ASTTOK_LEFTSHIFTASSIGN: return "NEON_ASTTOK_LEFTSHIFTASSIGN";
        case NEON_ASTTOK_GREATERTHAN: return "NEON_ASTTOK_GREATERTHAN";
        case NEON_ASTTOK_GREATER_EQ: return "NEON_ASTTOK_GREATER_EQ";
        case NEON_ASTTOK_RIGHTSHIFT: return "NEON_ASTTOK_RIGHTSHIFT";
        case NEON_ASTTOK_RIGHTSHIFTASSIGN: return "NEON_ASTTOK_RIGHTSHIFTASSIGN";
        case NEON_ASTTOK_MODULO: return "NEON_ASTTOK_MODULO";
        case NEON_ASTTOK_PERCENT_EQ: return "NEON_ASTTOK_PERCENT_EQ";
        case NEON_ASTTOK_AMP: return "NEON_ASTTOK_AMP";
        case NEON_ASTTOK_AMP_EQ: return "NEON_ASTTOK_AMP_EQ";
        case NEON_ASTTOK_BAR: return "NEON_ASTTOK_BAR";
        case NEON_ASTTOK_BAR_EQ: return "NEON_ASTTOK_BAR_EQ";
        case NEON_ASTTOK_TILDE: return "NEON_ASTTOK_TILDE";
        case NEON_ASTTOK_TILDE_EQ: return "NEON_ASTTOK_TILDE_EQ";
        case NEON_ASTTOK_XOR: return "NEON_ASTTOK_XOR";
        case NEON_ASTTOK_XOR_EQ: return "NEON_ASTTOK_XOR_EQ";
        case NEON_ASTTOK_QUESTION: return "NEON_ASTTOK_QUESTION";
        case NEON_ASTTOK_KWAND: return "NEON_ASTTOK_KWAND";
        case NEON_ASTTOK_KWAS: return "NEON_ASTTOK_KWAS";
        case NEON_ASTTOK_KWASSERT: return "NEON_ASTTOK_KWASSERT";
        case NEON_ASTTOK_KWBREAK: return "NEON_ASTTOK_KWBREAK";
        case NEON_ASTTOK_KWCATCH: return "NEON_ASTTOK_KWCATCH";
        case NEON_ASTTOK_KWCLASS: return "NEON_ASTTOK_KWCLASS";
        case NEON_ASTTOK_KWCONTINUE: return "NEON_ASTTOK_KWCONTINUE";
        case NEON_ASTTOK_KWFUNCTION: return "NEON_ASTTOK_KWFUNCTION";
        case NEON_ASTTOK_KWDEFAULT: return "NEON_ASTTOK_KWDEFAULT";
        case NEON_ASTTOK_KWTHROW: return "NEON_ASTTOK_KWTHROW";
        case NEON_ASTTOK_KWDO: return "NEON_ASTTOK_KWDO";
        case NEON_ASTTOK_KWECHO: return "NEON_ASTTOK_KWECHO";
        case NEON_ASTTOK_KWELSE: return "NEON_ASTTOK_KWELSE";
        case NEON_ASTTOK_KWFALSE: return "NEON_ASTTOK_KWFALSE";
        case NEON_ASTTOK_KWFINALLY: return "NEON_ASTTOK_KWFINALLY";
        case NEON_ASTTOK_KWFOREACH: return "NEON_ASTTOK_KWFOREACH";
        case NEON_ASTTOK_KWIF: return "NEON_ASTTOK_KWIF";
        case NEON_ASTTOK_KWIMPORT: return "NEON_ASTTOK_KWIMPORT";
        case NEON_ASTTOK_KWIN: return "NEON_ASTTOK_KWIN";
        case NEON_ASTTOK_KWFOR: return "NEON_ASTTOK_KWFOR";
        case NEON_ASTTOK_KWNULL: return "NEON_ASTTOK_KWNULL";
        case NEON_ASTTOK_KWNEW: return "NEON_ASTTOK_KWNEW";
        case NEON_ASTTOK_KWOR: return "NEON_ASTTOK_KWOR";
        case NEON_ASTTOK_KWSUPER: return "NEON_ASTTOK_KWSUPER";
        case NEON_ASTTOK_KWRETURN: return "NEON_ASTTOK_KWRETURN";
        case NEON_ASTTOK_KWTHIS: return "NEON_ASTTOK_KWTHIS";
        case NEON_ASTTOK_KWSTATIC: return "NEON_ASTTOK_KWSTATIC";
        case NEON_ASTTOK_KWTRUE: return "NEON_ASTTOK_KWTRUE";
        case NEON_ASTTOK_KWTRY: return "NEON_ASTTOK_KWTRY";
        case NEON_ASTTOK_KWSWITCH: return "NEON_ASTTOK_KWSWITCH";
        case NEON_ASTTOK_KWVAR: return "NEON_ASTTOK_KWVAR";
        case NEON_ASTTOK_KWCASE: return "NEON_ASTTOK_KWCASE";
        case NEON_ASTTOK_KWWHILE: return "NEON_ASTTOK_KWWHILE";
        case NEON_ASTTOK_LITERAL: return "NEON_ASTTOK_LITERAL";
        case NEON_ASTTOK_LITNUMREG: return "NEON_ASTTOK_LITNUMREG";
        case NEON_ASTTOK_LITNUMBIN: return "NEON_ASTTOK_LITNUMBIN";
        case NEON_ASTTOK_LITNUMOCT: return "NEON_ASTTOK_LITNUMOCT";
        case NEON_ASTTOK_LITNUMHEX: return "NEON_ASTTOK_LITNUMHEX";
        case NEON_ASTTOK_IDENTNORMAL: return "NEON_ASTTOK_IDENTNORMAL";
        case NEON_ASTTOK_DECORATOR: return "NEON_ASTTOK_DECORATOR";
        case NEON_ASTTOK_INTERPOLATION: return "NEON_ASTTOK_INTERPOLATION";
        case NEON_ASTTOK_EOF: return "NEON_ASTTOK_EOF";
        case NEON_ASTTOK_ERROR: return "NEON_ASTTOK_ERROR";
        case NEON_ASTTOK_KWEMPTY: return "NEON_ASTTOK_KWEMPTY";
        case NEON_ASTTOK_UNDEFINED: return "NEON_ASTTOK_UNDEFINED";
        case NEON_ASTTOK_TOKCOUNT: return "NEON_ASTTOK_TOKCOUNT";
    }
    return "?invalid?";
}

char nn_astlex_advance(NNAstLexer* lex)
{
    lex->sourceptr++;
    if(lex->sourceptr[-1] == '\n')
    {
        lex->line++;
    }
    return lex->sourceptr[-1];
}

bool nn_astlex_match(NNAstLexer* lex, char expected)
{
    if(nn_astlex_isatend(lex))
    {
        return false;
    }
    if(*lex->sourceptr != expected)
    {
        return false;
    }
    lex->sourceptr++;
    if(lex->sourceptr[-1] == '\n')
    {
        lex->line++;
    }
    return true;
}

char nn_astlex_peekcurr(NNAstLexer* lex)
{
    return *lex->sourceptr;
}

char nn_astlex_peekprev(NNAstLexer* lex)
{
    return lex->sourceptr[-1];
}

char nn_astlex_peeknext(NNAstLexer* lex)
{
    if(nn_astlex_isatend(lex))
    {
        return '\0';
    }
    return lex->sourceptr[1];
}

NNAstToken nn_astlex_skipblockcomments(NNAstLexer* lex)
{
    int nesting;
    nesting = 1;
    while(nesting > 0)
    {
        if(nn_astlex_isatend(lex))
        {
            return nn_astlex_errortoken(lex, "unclosed block comment");
        }
        /* internal comment open */
        if(nn_astlex_peekcurr(lex) == '/' && nn_astlex_peeknext(lex) == '*')
        {
            nn_astlex_advance(lex);
            nn_astlex_advance(lex);
            nesting++;
            continue;
        }
        /* comment close */
        if(nn_astlex_peekcurr(lex) == '*' && nn_astlex_peeknext(lex) == '/')
        {
            nn_astlex_advance(lex);
            nn_astlex_advance(lex);
            nesting--;
            continue;
        }
        /* regular comment body */
        nn_astlex_advance(lex);
    }
    #if defined(NEON_PLAT_ISWINDOWS)
        #if 0
            nn_astlex_advance(lex);
        #endif
    #endif
    return nn_astlex_maketoken(lex, NEON_ASTTOK_UNDEFINED);
}

NNAstToken nn_astlex_skipspace(NNAstLexer* lex)
{
    char c;
    NNAstToken result;
    result.isglobal = false;
    for(;;)
    {
        c = nn_astlex_peekcurr(lex);
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
            {
                nn_astlex_advance(lex);
            }
            break;
            /*
            case '\n':
                {
                    lex->line++;
                    nn_astlex_advance(lex);
                }
                break;
            */
            /*
            case '#':
                // single line comment
                {
                    while(nn_astlex_peekcurr(lex) != '\n' && !nn_astlex_isatend(lex))
                        nn_astlex_advance(lex);

                }
                break;
            */
            case '/':
            {
                if(nn_astlex_peeknext(lex) == '/')
                {
                    while(nn_astlex_peekcurr(lex) != '\n' && !nn_astlex_isatend(lex))
                    {
                        nn_astlex_advance(lex);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_UNDEFINED);
                }
                else if(nn_astlex_peeknext(lex) == '*')
                {
                    nn_astlex_advance(lex);
                    nn_astlex_advance(lex);
                    result = nn_astlex_skipblockcomments(lex);
                    if(result.type != NEON_ASTTOK_UNDEFINED)
                    {
                        return result;
                    }
                    break;
                }
                else
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_UNDEFINED);
                }
            }
            break;
            /* exit as soon as we see a non-whitespace... */
            default:
                goto finished;
                break;
        }
    }
    finished:
    return nn_astlex_maketoken(lex, NEON_ASTTOK_UNDEFINED);
}

NNAstToken nn_astlex_scanstring(NNAstLexer* lex, char quote, bool withtemplate)
{
    NNAstToken tkn;
    NEON_ASTDEBUG(lex->pvm, "quote=[%c] withtemplate=%d", quote, withtemplate);
    while(nn_astlex_peekcurr(lex) != quote && !nn_astlex_isatend(lex))
    {
        if(withtemplate)
        {
            /* interpolation started */
            if(nn_astlex_peekcurr(lex) == '$' && nn_astlex_peeknext(lex) == '{' && nn_astlex_peekprev(lex) != '\\')
            {
                if(lex->tplstringcount - 1 < NEON_CFG_ASTMAXSTRTPLDEPTH)
                {
                    lex->tplstringcount++;
                    lex->tplstringbuffer[lex->tplstringcount] = (int)quote;
                    lex->sourceptr++;
                    tkn = nn_astlex_maketoken(lex, NEON_ASTTOK_INTERPOLATION);
                    lex->sourceptr++;
                    return tkn;
                }
                return nn_astlex_errortoken(lex, "maximum interpolation nesting of %d exceeded by %d", NEON_CFG_ASTMAXSTRTPLDEPTH,
                    NEON_CFG_ASTMAXSTRTPLDEPTH - lex->tplstringcount + 1);
            }
        }
        if(nn_astlex_peekcurr(lex) == '\\' && (nn_astlex_peeknext(lex) == quote || nn_astlex_peeknext(lex) == '\\'))
        {
            nn_astlex_advance(lex);
        }
        nn_astlex_advance(lex);
    }
    if(nn_astlex_isatend(lex))
    {
        return nn_astlex_errortoken(lex, "unterminated string (opening quote not matched)");
    }
    /* the closing quote */
    nn_astlex_match(lex, quote);
    return nn_astlex_maketoken(lex, NEON_ASTTOK_LITERAL);
}

NNAstToken nn_astlex_scannumber(NNAstLexer* lex)
{
    NEON_ASTDEBUG(lex->pvm, "");
    /* handle binary, octal and hexadecimals */
    if(nn_astlex_peekprev(lex) == '0')
    {
        /* binary number */
        if(nn_astlex_match(lex, 'b'))
        {
            while(nn_astutil_isbinary(nn_astlex_peekcurr(lex)))
            {
                nn_astlex_advance(lex);
            }
            return nn_astlex_maketoken(lex, NEON_ASTTOK_LITNUMBIN);
        }
        else if(nn_astlex_match(lex, 'c'))
        {
            while(nn_astutil_isoctal(nn_astlex_peekcurr(lex)))
            {
                nn_astlex_advance(lex);
            }
            return nn_astlex_maketoken(lex, NEON_ASTTOK_LITNUMOCT);
        }
        else if(nn_astlex_match(lex, 'x'))
        {
            while(nn_astutil_ishexadecimal(nn_astlex_peekcurr(lex)))
            {
                nn_astlex_advance(lex);
            }
            return nn_astlex_maketoken(lex, NEON_ASTTOK_LITNUMHEX);
        }
    }
    while(nn_astutil_isdigit(nn_astlex_peekcurr(lex)))
    {
        nn_astlex_advance(lex);
    }
    /* dots(.) are only valid here when followed by a digit */
    if(nn_astlex_peekcurr(lex) == '.' && nn_astutil_isdigit(nn_astlex_peeknext(lex)))
    {
        nn_astlex_advance(lex);
        while(nn_astutil_isdigit(nn_astlex_peekcurr(lex)))
        {
            nn_astlex_advance(lex);
        }
        /*
        // E or e are only valid here when followed by a digit and occurring after a dot
        */
        if((nn_astlex_peekcurr(lex) == 'e' || nn_astlex_peekcurr(lex) == 'E') && (nn_astlex_peeknext(lex) == '+' || nn_astlex_peeknext(lex) == '-'))
        {
            nn_astlex_advance(lex);
            nn_astlex_advance(lex);
            while(nn_astutil_isdigit(nn_astlex_peekcurr(lex)))
            {
                nn_astlex_advance(lex);
            }
        }
    }
    return nn_astlex_maketoken(lex, NEON_ASTTOK_LITNUMREG);
}

NNAstTokType nn_astlex_getidenttype(NNAstLexer* lex)
{
    static const struct
    {
        const char* str;
        int tokid;
    }
    keywords[] =
    {
        { "and", NEON_ASTTOK_KWAND },
        { "assert", NEON_ASTTOK_KWASSERT },
        { "as", NEON_ASTTOK_KWAS },
        { "break", NEON_ASTTOK_KWBREAK },
        { "catch", NEON_ASTTOK_KWCATCH },
        { "class", NEON_ASTTOK_KWCLASS },
        { "continue", NEON_ASTTOK_KWCONTINUE },
        { "default", NEON_ASTTOK_KWDEFAULT },
        { "def", NEON_ASTTOK_KWFUNCTION },
        { "function", NEON_ASTTOK_KWFUNCTION },
        { "throw", NEON_ASTTOK_KWTHROW },
        { "do", NEON_ASTTOK_KWDO },
        { "echo", NEON_ASTTOK_KWECHO },
        { "else", NEON_ASTTOK_KWELSE },
        { "empty", NEON_ASTTOK_KWEMPTY },
        { "false", NEON_ASTTOK_KWFALSE },
        { "finally", NEON_ASTTOK_KWFINALLY },
        { "foreach", NEON_ASTTOK_KWFOREACH },
        { "if", NEON_ASTTOK_KWIF },
        { "import", NEON_ASTTOK_KWIMPORT },
        { "in", NEON_ASTTOK_KWIN },
        { "for", NEON_ASTTOK_KWFOR },
        { "null", NEON_ASTTOK_KWNULL },
        { "new", NEON_ASTTOK_KWNEW },
        { "or", NEON_ASTTOK_KWOR },
        { "super", NEON_ASTTOK_KWSUPER },
        { "return", NEON_ASTTOK_KWRETURN },
        { "this", NEON_ASTTOK_KWTHIS },
        { "static", NEON_ASTTOK_KWSTATIC },
        { "true", NEON_ASTTOK_KWTRUE },
        { "try", NEON_ASTTOK_KWTRY },
        { "typeof", NEON_ASTTOK_KWTYPEOF },
        { "switch", NEON_ASTTOK_KWSWITCH },
        { "case", NEON_ASTTOK_KWCASE },
        { "var", NEON_ASTTOK_KWVAR },
        { "while", NEON_ASTTOK_KWWHILE },
        { NULL, (NNAstTokType)0 }
    };
    size_t i;
    size_t kwlen;
    size_t ofs;
    const char* kwtext;
    for(i = 0; keywords[i].str != NULL; i++)
    {
        kwtext = keywords[i].str;
        kwlen = strlen(kwtext);
        ofs = (lex->sourceptr - lex->start);
        if(ofs == kwlen)
        {
            if(memcmp(lex->start, kwtext, kwlen) == 0)
            {
                return (NNAstTokType)keywords[i].tokid;
            }
        }
    }
    return NEON_ASTTOK_IDENTNORMAL;
}

NNAstToken nn_astlex_scanident(NNAstLexer* lex, bool isdollar)
{
    int cur;
    NNAstToken tok;
    cur = nn_astlex_peekcurr(lex);
    if(cur == '$')
    {
        nn_astlex_advance(lex);
    }
    while(true)
    {
        cur = nn_astlex_peekcurr(lex);
        if(nn_astutil_isalpha(cur) || nn_astutil_isdigit(cur))
        {
            nn_astlex_advance(lex);
        }
        else
        {
            break;
        }
    }
    tok = nn_astlex_maketoken(lex, nn_astlex_getidenttype(lex));
    tok.isglobal = isdollar;
    return tok;
}

NNAstToken nn_astlex_scandecorator(NNAstLexer* lex)
{
    while(nn_astutil_isalpha(nn_astlex_peekcurr(lex)) || nn_astutil_isdigit(nn_astlex_peekcurr(lex)))
    {
        nn_astlex_advance(lex);
    }
    return nn_astlex_maketoken(lex, NEON_ASTTOK_DECORATOR);
}

NNAstToken nn_astlex_scantoken(NNAstLexer* lex)
{
    char c;
    bool isdollar;
    NNAstToken tk;
    NNAstToken token;
    tk = nn_astlex_skipspace(lex);
    if(tk.type != NEON_ASTTOK_UNDEFINED)
    {
        return tk;
    }
    lex->start = lex->sourceptr;
    if(nn_astlex_isatend(lex))
    {
        return nn_astlex_maketoken(lex, NEON_ASTTOK_EOF);
    }
    c = nn_astlex_advance(lex);
    if(nn_astutil_isdigit(c))
    {
        return nn_astlex_scannumber(lex);
    }
    else if(nn_astutil_isalpha(c) || (c == '$'))
    {
        isdollar = (c == '$');
        return nn_astlex_scanident(lex, isdollar);
    }
    switch(c)
    {
        case '(':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_PARENOPEN);
            }
            break;
        case ')':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_PARENCLOSE);
            }
            break;
        case '[':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_BRACKETOPEN);
            }
            break;
        case ']':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_BRACKETCLOSE);
            }
            break;
        case '{':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_BRACEOPEN);
            }
            break;
        case '}':
            {
                if(lex->tplstringcount > -1)
                {
                    token = nn_astlex_scanstring(lex, (char)lex->tplstringbuffer[lex->tplstringcount], true);
                    lex->tplstringcount--;
                    return token;
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_BRACECLOSE);
            }
            break;
        case ';':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_SEMICOLON);
            }
            break;
        case '\\':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_BACKSLASH);
            }
            break;
        case ':':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_COLON);
            }
            break;
        case ',':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_COMMA);
            }
            break;
        case '@':
            {
                if(!nn_astutil_isalpha(nn_astlex_peekcurr(lex)))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_AT);
                }
                return nn_astlex_scandecorator(lex);
            }
            break;
        case '!':
            {
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_NOTEQUAL);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_EXCLMARK);

            }
            break;
        case '.':
            {
                if(nn_astlex_match(lex, '.'))
                {
                    if(nn_astlex_match(lex, '.'))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_TRIPLEDOT);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_DOUBLEDOT);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_DOT);
            }
            break;
        case '+':
        {
            if(nn_astlex_match(lex, '+'))
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_INCREMENT);
            }
            if(nn_astlex_match(lex, '='))
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_PLUSASSIGN);
            }
            else
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_PLUS);
            }
        }
        break;
        case '-':
            {
                if(nn_astlex_match(lex, '-'))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_DECREMENT);
                }
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_MINUSASSIGN);
                }
                else
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_MINUS);
                }
            }
            break;
        case '*':
            {
                if(nn_astlex_match(lex, '*'))
                {
                    if(nn_astlex_match(lex, '='))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_POWASSIGN);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_POWEROF);
                }
                else
                {
                    if(nn_astlex_match(lex, '='))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_MULTASSIGN);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_MULTIPLY);
                }
            }
            break;
        case '/':
            {
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_DIVASSIGN);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_DIVIDE);
            }
            break;
        case '=':
            {
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_EQUAL);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_ASSIGN);
            }        
            break;
        case '<':
            {
                if(nn_astlex_match(lex, '<'))
                {
                    if(nn_astlex_match(lex, '='))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_LEFTSHIFTASSIGN);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_LEFTSHIFT);
                }
                else
                {
                    if(nn_astlex_match(lex, '='))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_LESSEQUAL);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_LESSTHAN);

                }
            }
            break;
        case '>':
            {
                if(nn_astlex_match(lex, '>'))
                {
                    if(nn_astlex_match(lex, '='))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_RIGHTSHIFTASSIGN);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_RIGHTSHIFT);
                }
                else
                {
                    if(nn_astlex_match(lex, '='))
                    {
                        return nn_astlex_maketoken(lex, NEON_ASTTOK_GREATER_EQ);
                    }
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_GREATERTHAN);
                }
            }
            break;
        case '%':
            {
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_PERCENT_EQ);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_MODULO);
            }
            break;
        case '&':
            {
                if(nn_astlex_match(lex, '&'))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_KWAND);
                }
                else if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_AMP_EQ);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_AMP);
            }
            break;
        case '|':
            {
                if(nn_astlex_match(lex, '|'))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_KWOR);
                }
                else if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_BAR_EQ);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_BAR);
            }
            break;
        case '~':
            {
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_TILDE_EQ);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_TILDE);
            }
            break;
        case '^':
            {
                if(nn_astlex_match(lex, '='))
                {
                    return nn_astlex_maketoken(lex, NEON_ASTTOK_XOR_EQ);
                }
                return nn_astlex_maketoken(lex, NEON_ASTTOK_XOR);
            }
            break;
        case '\n':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_NEWLINE);
            }
            break;
        case '"':
            {
                return nn_astlex_scanstring(lex, '"', true);
            }
            break;
        case '\'':
            {
                return nn_astlex_scanstring(lex, '\'', false);
            }
            break;
        case '?':
            {
                return nn_astlex_maketoken(lex, NEON_ASTTOK_QUESTION);
            }
            break;
        /*
        // --- DO NOT MOVE ABOVE OR BELOW THE DEFAULT CASE ---
        // fall-through tokens goes here... this tokens are only valid
        // when the carry another token with them...
        // be careful not to add break after them so that they may use the default
        // case.
        */
        default:
            break;
    }
    return nn_astlex_errortoken(lex, "unexpected character %c", c);
}

NNAstParser* nn_astparser_make(NNState* state, NNAstLexer* lexer, NNObjModule* module, bool keeplast)
{
    NNAstParser* parser;
    NEON_ASTDEBUG(state, "");
    parser = (NNAstParser*)nn_gcmem_allocate(state, sizeof(NNAstParser), 1);
    parser->pvm = state;
    parser->lexer = lexer;
    parser->currentfunccompiler = NULL;
    parser->haderror = false;
    parser->panicmode = false;
    parser->blockcount = 0;
    parser->replcanecho = false;
    parser->isreturning = false;
    parser->istrying = false;
    parser->compcontext = NEON_COMPCONTEXT_NONE;
    parser->innermostloopstart = -1;
    parser->innermostloopscopedepth = 0;
    parser->currentclasscompiler = NULL;
    parser->currentmodule = module;
    parser->keeplastvalue = keeplast;
    parser->lastwasstatement = false;
    parser->infunction = false;
    parser->currentfile = parser->currentmodule->physicalpath->sbuf->data;
    return parser;
}

void nn_astparser_destroy(NNState* state, NNAstParser* parser)
{
    nn_gcmem_release(state, parser, sizeof(NNAstParser));
}

NNBlob* nn_astparser_currentblob(NNAstParser* prs)
{
    return &prs->currentfunccompiler->targetfunc->blob;
}

bool nn_astparser_raiseerroratv(NNAstParser* prs, NNAstToken* t, const char* message, va_list args)
{
    fflush(stdout);
    /*
    // do not cascade error
    // suppress error if already in panic mode
    */
    if(prs->panicmode)
    {
        return false;
    }
    prs->panicmode = true;
    fprintf(stderr, "SyntaxError");
    if(t->type == NEON_ASTTOK_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if(t->type == NEON_ASTTOK_ERROR)
    {
        /* do nothing */
    }
    else
    {
        if(t->length == 1 && *t->start == '\n')
        {
            fprintf(stderr, " at newline");
        }
        else
        {
            fprintf(stderr, " at '%.*s'", t->length, t->start);
        }
    }
    fprintf(stderr, ": ");
    vfprintf(stderr, message, args);
    fputs("\n", stderr);
    fprintf(stderr, "  %s:%d\n", prs->currentmodule->physicalpath->sbuf->data, t->line);
    prs->haderror = true;
    return false;
}

bool nn_astparser_raiseerror(NNAstParser* prs, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    nn_astparser_raiseerroratv(prs, &prs->prevtoken, message, args);
    va_end(args);
    return false;
}

bool nn_astparser_raiseerroratcurrent(NNAstParser* prs, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    nn_astparser_raiseerroratv(prs, &prs->currtoken, message, args);
    va_end(args);
    return false;
}

void nn_astparser_advance(NNAstParser* prs)
{
    prs->prevtoken = prs->currtoken;
    while(true)
    {
        prs->currtoken = nn_astlex_scantoken(prs->lexer);
        if(prs->currtoken.type != NEON_ASTTOK_ERROR)
        {
            break;
        }
        nn_astparser_raiseerroratcurrent(prs, prs->currtoken.start);
    }
}

bool nn_astparser_consume(NNAstParser* prs, NNAstTokType t, const char* message)
{
    if(prs->currtoken.type == t)
    {
        nn_astparser_advance(prs);
        return true;
    }
    return nn_astparser_raiseerroratcurrent(prs, message);
}

void nn_astparser_consumeor(NNAstParser* prs, const char* message, const NNAstTokType* ts, int count)
{
    int i;
    for(i = 0; i < count; i++)
    {
        if(prs->currtoken.type == ts[i])
        {
            nn_astparser_advance(prs);
            return;
        }
    }
    nn_astparser_raiseerroratcurrent(prs, message);
}

bool nn_astparser_checknumber(NNAstParser* prs)
{
    NNAstTokType t;
    t = prs->prevtoken.type;
    if(t == NEON_ASTTOK_LITNUMREG || t == NEON_ASTTOK_LITNUMOCT || t == NEON_ASTTOK_LITNUMBIN || t == NEON_ASTTOK_LITNUMHEX)
    {
        return true;
    }
    return false;
}

bool nn_astparser_check(NNAstParser* prs, NNAstTokType t)
{
    return prs->currtoken.type == t;
}

bool nn_astparser_match(NNAstParser* prs, NNAstTokType t)
{
    if(!nn_astparser_check(prs, t))
    {
        return false;
    }
    nn_astparser_advance(prs);
    return true;
}

void nn_astparser_runparser(NNAstParser* parser)
{
    nn_astparser_advance(parser);
    nn_astparser_ignorewhitespace(parser);
    while(!nn_astparser_match(parser, NEON_ASTTOK_EOF))
    {
        nn_astparser_parsedeclaration(parser);
    }
}

void nn_astparser_parsedeclaration(NNAstParser* prs)
{
    nn_astparser_ignorewhitespace(prs);
    if(nn_astparser_match(prs, NEON_ASTTOK_KWCLASS))
    {
        nn_astparser_parseclassdeclaration(prs, true);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWFUNCTION))
    {
        nn_astparser_parsefuncdecl(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWVAR))
    {
        nn_astparser_parsevardecl(prs, false);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_BRACEOPEN))
    {
        if(!nn_astparser_check(prs, NEON_ASTTOK_NEWLINE) && prs->currentfunccompiler->scopedepth == 0)
        {
            nn_astparser_parseexprstmt(prs, false, true);
        }
        else
        {
            nn_astparser_scopebegin(prs);
            nn_astparser_parseblock(prs);
            nn_astparser_scopeend(prs);
        }
    }
    else
    {
        nn_astparser_parsestmt(prs);
    }
    nn_astparser_ignorewhitespace(prs);
    if(prs->panicmode)
    {
        nn_astparser_synchronize(prs);
    }
    nn_astparser_ignorewhitespace(prs);
}

void nn_astparser_parsestmt(NNAstParser* prs)
{
    prs->replcanecho = false;
    nn_astparser_ignorewhitespace(prs);
    if(nn_astparser_match(prs, NEON_ASTTOK_KWECHO))
    {
        nn_astparser_parseechostmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWIF))
    {
        nn_astparser_parseifstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWDO))
    {
        nn_astparser_parsedo_whilestmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWWHILE))
    {
        nn_astparser_parsewhilestmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWFOR))
    {
        nn_astparser_parseforstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWFOREACH))
    {
        nn_astparser_parseforeachstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWSWITCH))
    {
        nn_astparser_parseswitchstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWCONTINUE))
    {
        nn_astparser_parsecontinuestmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWBREAK))
    {
        nn_astparser_parsebreakstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWRETURN))
    {
        nn_astparser_parsereturnstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWASSERT))
    {
        nn_astparser_parseassertstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWTHROW))
    {
        nn_astparser_parsethrowstmt(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_BRACEOPEN))
    {
        nn_astparser_scopebegin(prs);
        nn_astparser_parseblock(prs);
        nn_astparser_scopeend(prs);
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWTRY))
    {
        nn_astparser_parsetrystmt(prs);
    }
    else
    {
        nn_astparser_parseexprstmt(prs, false, false);
    }
    nn_astparser_ignorewhitespace(prs);
}

void nn_astparser_consumestmtend(NNAstParser* prs)
{
    /* allow block last statement to omit statement end */
    if(prs->blockcount > 0 && nn_astparser_check(prs, NEON_ASTTOK_BRACECLOSE))
    {
        return;
    }
    if(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON))
    {
        while(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON) || nn_astparser_match(prs, NEON_ASTTOK_NEWLINE))
        {
        }
        return;
    }
    if(nn_astparser_match(prs, NEON_ASTTOK_EOF) || prs->prevtoken.type == NEON_ASTTOK_EOF)
    {
        return;
    }
    /* nn_astparser_consume(prs, NEON_ASTTOK_NEWLINE, "end of statement expected"); */
    while(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON) || nn_astparser_match(prs, NEON_ASTTOK_NEWLINE))
    {
    }
}

void nn_astparser_ignorewhitespace(NNAstParser* prs)
{
    while(true)
    {
        if(nn_astparser_check(prs, NEON_ASTTOK_NEWLINE))
        {
            nn_astparser_advance(prs);
        }
        else
        {
            break;
        }
    }
}

int nn_astparser_getcodeargscount(const NNInstruction* bytecode, const NNValue* constants, int ip)
{
    int constant;
    NNOpCode code;
    NNObjFuncScript* fn;
    code = (NNOpCode)bytecode[ip].code;
    switch(code)
    {
        case NEON_OP_EQUAL:
        case NEON_OP_PRIMGREATER:
        case NEON_OP_PRIMLESSTHAN:
        case NEON_OP_PUSHNULL:
        case NEON_OP_PUSHTRUE:
        case NEON_OP_PUSHFALSE:
        case NEON_OP_PRIMADD:
        case NEON_OP_PRIMSUBTRACT:
        case NEON_OP_PRIMMULTIPLY:
        case NEON_OP_PRIMDIVIDE:
        case NEON_OP_PRIMFLOORDIVIDE:
        case NEON_OP_PRIMMODULO:
        case NEON_OP_PRIMPOW:
        case NEON_OP_PRIMNEGATE:
        case NEON_OP_PRIMNOT:
        case NEON_OP_ECHO:
        case NEON_OP_TYPEOF:
        case NEON_OP_POPONE:
        case NEON_OP_UPVALUECLOSE:
        case NEON_OP_DUPONE:
        case NEON_OP_RETURN:
        case NEON_OP_CLASSINHERIT:
        case NEON_OP_CLASSGETSUPER:
        case NEON_OP_PRIMAND:
        case NEON_OP_PRIMOR:
        case NEON_OP_PRIMBITXOR:
        case NEON_OP_PRIMSHIFTLEFT:
        case NEON_OP_PRIMSHIFTRIGHT:
        case NEON_OP_PRIMBITNOT:
        case NEON_OP_PUSHONE:
        case NEON_OP_INDEXSET:
        case NEON_OP_ASSERT:
        case NEON_OP_EXTHROW:
        case NEON_OP_EXPOPTRY:
        case NEON_OP_MAKERANGE:
        case NEON_OP_STRINGIFY:
        case NEON_OP_PUSHEMPTY:
        case NEON_OP_EXPUBLISHTRY:
        case NEON_OP_CLASSGETTHIS:
            return 0;
        case NEON_OP_CALLFUNCTION:
        case NEON_OP_CLASSINVOKESUPERSELF:
        case NEON_OP_INDEXGET:
        case NEON_OP_INDEXGETRANGED:
            return 1;
        case NEON_OP_GLOBALDEFINE:
        case NEON_OP_GLOBALGET:
        case NEON_OP_GLOBALSET:
        case NEON_OP_LOCALGET:
        case NEON_OP_LOCALSET:
        case NEON_OP_FUNCARGSET:
        case NEON_OP_FUNCARGGET:
        case NEON_OP_UPVALUEGET:
        case NEON_OP_UPVALUESET:
        case NEON_OP_JUMPIFFALSE:
        case NEON_OP_JUMPNOW:
        case NEON_OP_BREAK_PL:
        case NEON_OP_LOOP:
        case NEON_OP_PUSHCONSTANT:
        case NEON_OP_POPN:
        case NEON_OP_MAKECLASS:
        case NEON_OP_PROPERTYGET:
        case NEON_OP_PROPERTYGETSELF:
        case NEON_OP_PROPERTYSET:
        case NEON_OP_MAKEARRAY:
        case NEON_OP_MAKEDICT:
        case NEON_OP_IMPORTIMPORT:
        case NEON_OP_SWITCH:
        case NEON_OP_MAKEMETHOD:
        #if 0
        case NEON_OP_FUNCOPTARG:
        #endif
            return 2;
        case NEON_OP_CALLMETHOD:
        case NEON_OP_CLASSINVOKETHIS:
        case NEON_OP_CLASSINVOKESUPER:
        case NEON_OP_CLASSPROPERTYDEFINE:
            return 3;
        case NEON_OP_EXTRY:
            return 6;
        case NEON_OP_MAKECLOSURE:
            {
                constant = (bytecode[ip + 1].code << 8) | bytecode[ip + 2].code;
                fn = nn_value_asfuncscript(constants[constant]);
                /* There is two byte for the constant, then three for each up value. */
                return 2 + (fn->upvalcount * 3);
            }
            break;
        default:
            break;
    }
    return 0;
}

void nn_astemit_emit(NNAstParser* prs, uint8_t byte, int line, bool isop)
{
    NNInstruction ins;
    ins.code = byte;
    ins.srcline = line;
    ins.isop = isop;
    nn_blob_push(prs->pvm, nn_astparser_currentblob(prs), ins);
}

void nn_astemit_patchat(NNAstParser* prs, size_t idx, uint8_t byte)
{
    nn_astparser_currentblob(prs)->instrucs[idx].code = byte;
}

void nn_astemit_emitinstruc(NNAstParser* prs, uint8_t byte)
{
    nn_astemit_emit(prs, byte, prs->prevtoken.line, true);
}

void nn_astemit_emit1byte(NNAstParser* prs, uint8_t byte)
{
    nn_astemit_emit(prs, byte, prs->prevtoken.line, false);
}

void nn_astemit_emit1short(NNAstParser* prs, uint16_t byte)
{
    nn_astemit_emit(prs, (byte >> 8) & 0xff, prs->prevtoken.line, false);
    nn_astemit_emit(prs, byte & 0xff, prs->prevtoken.line, false);
}

void nn_astemit_emit2byte(NNAstParser* prs, uint8_t byte, uint8_t byte2)
{
    nn_astemit_emit(prs, byte, prs->prevtoken.line, false);
    nn_astemit_emit(prs, byte2, prs->prevtoken.line, false);
}

void nn_astemit_emitbyteandshort(NNAstParser* prs, uint8_t byte, uint16_t byte2)
{
    nn_astemit_emit(prs, byte, prs->prevtoken.line, false);
    nn_astemit_emit(prs, (byte2 >> 8) & 0xff, prs->prevtoken.line, false);
    nn_astemit_emit(prs, byte2 & 0xff, prs->prevtoken.line, false);
}

void nn_astemit_emitloop(NNAstParser* prs, int loopstart)
{
    int offset;
    nn_astemit_emitinstruc(prs, NEON_OP_LOOP);
    offset = nn_astparser_currentblob(prs)->count - loopstart + 2;
    if(offset > UINT16_MAX)
    {
        nn_astparser_raiseerror(prs, "loop body too large");
    }
    nn_astemit_emit1byte(prs, (offset >> 8) & 0xff);
    nn_astemit_emit1byte(prs, offset & 0xff);
}

void nn_astemit_emitreturn(NNAstParser* prs)
{
    if(prs->istrying)
    {
        nn_astemit_emitinstruc(prs, NEON_OP_EXPOPTRY);
    }
    if(prs->currentfunccompiler->type == NEON_FUNCTYPE_INITIALIZER)
    {
        nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALGET, 0);
    }
    else
    {
        if(!prs->keeplastvalue || prs->lastwasstatement)
        {
            if(prs->currentfunccompiler->fromimport)
            {
                nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
            }
            else
            {
                nn_astemit_emitinstruc(prs, NEON_OP_PUSHEMPTY);
            }
        }
    }
    nn_astemit_emitinstruc(prs, NEON_OP_RETURN);
}

int nn_astparser_pushconst(NNAstParser* prs, NNValue value)
{
    int constant;
    constant = nn_blob_pushconst(prs->pvm, nn_astparser_currentblob(prs), value);
    if(constant >= UINT16_MAX)
    {
        nn_astparser_raiseerror(prs, "too many constants in current scope");
        return 0;
    }
    return constant;
}

void nn_astemit_emitconst(NNAstParser* prs, NNValue value)
{
    int constant;
    constant = nn_astparser_pushconst(prs, value);
    nn_astemit_emitbyteandshort(prs, NEON_OP_PUSHCONSTANT, (uint16_t)constant);
}

int nn_astemit_emitjump(NNAstParser* prs, uint8_t instruction)
{
    nn_astemit_emitinstruc(prs, instruction);
    /* placeholders */
    nn_astemit_emit1byte(prs, 0xff);
    nn_astemit_emit1byte(prs, 0xff);
    return nn_astparser_currentblob(prs)->count - 2;
}

int nn_astemit_emitswitch(NNAstParser* prs)
{
    nn_astemit_emitinstruc(prs, NEON_OP_SWITCH);
    /* placeholders */
    nn_astemit_emit1byte(prs, 0xff);
    nn_astemit_emit1byte(prs, 0xff);
    return nn_astparser_currentblob(prs)->count - 2;
}

int nn_astemit_emittry(NNAstParser* prs)
{
    nn_astemit_emitinstruc(prs, NEON_OP_EXTRY);
    /* type placeholders */
    nn_astemit_emit1byte(prs, 0xff);
    nn_astemit_emit1byte(prs, 0xff);
    /* handler placeholders */
    nn_astemit_emit1byte(prs, 0xff);
    nn_astemit_emit1byte(prs, 0xff);
    /* finally placeholders */
    nn_astemit_emit1byte(prs, 0xff);
    nn_astemit_emit1byte(prs, 0xff);
    return nn_astparser_currentblob(prs)->count - 6;
}

void nn_astemit_patchswitch(NNAstParser* prs, int offset, int constant)
{
    nn_astemit_patchat(prs, offset, (constant >> 8) & 0xff);
    nn_astemit_patchat(prs, offset + 1, constant & 0xff);
}

void nn_astemit_patchtry(NNAstParser* prs, int offset, int type, int address, int finally)
{
    /* patch type */
    nn_astemit_patchat(prs, offset, (type >> 8) & 0xff);
    nn_astemit_patchat(prs, offset + 1, type & 0xff);
    /* patch address */
    nn_astemit_patchat(prs, offset + 2, (address >> 8) & 0xff);
    nn_astemit_patchat(prs, offset + 3, address & 0xff);
    /* patch finally */
    nn_astemit_patchat(prs, offset + 4, (finally >> 8) & 0xff);
    nn_astemit_patchat(prs, offset + 5, finally & 0xff);
}

void nn_astemit_patchjump(NNAstParser* prs, int offset)
{
    /* -2 to adjust the bytecode for the offset itself */
    int jump;
    jump = nn_astparser_currentblob(prs)->count - offset - 2;
    if(jump > UINT16_MAX)
    {
        nn_astparser_raiseerror(prs, "body of conditional block too large");
    }
    nn_astemit_patchat(prs, offset, (jump >> 8) & 0xff);
    nn_astemit_patchat(prs, offset + 1, jump & 0xff);
}

void nn_astfunccompiler_init(NNAstParser* prs, NNAstFuncCompiler* compiler, NNFuncType type, bool isanon)
{
    bool candeclthis;
    NNPrinter wtmp;
    NNAstLocal* local;
    NNObjString* fname;
    compiler->enclosing = prs->currentfunccompiler;
    compiler->targetfunc = NULL;
    compiler->type = type;
    compiler->localcount = 0;
    compiler->scopedepth = 0;
    compiler->handlercount = 0;
    compiler->fromimport = false;
    compiler->targetfunc = nn_object_makefuncscript(prs->pvm, prs->currentmodule, type);
    prs->currentfunccompiler = compiler;
    if(type != NEON_FUNCTYPE_SCRIPT)
    {
        nn_vm_stackpush(prs->pvm, nn_value_fromobject(compiler->targetfunc));
        if(isanon)
        {
            nn_printer_makestackstring(prs->pvm, &wtmp);
            nn_printer_writefmt(&wtmp, "anonymous@[%s:%d]", prs->currentfile, prs->prevtoken.line);
            fname = nn_printer_takestring(&wtmp);
        }
        else
        {
            fname = nn_string_copylen(prs->pvm, prs->prevtoken.start, prs->prevtoken.length);
        }
        prs->currentfunccompiler->targetfunc->name = fname;
        nn_vm_stackpop(prs->pvm);
    }
    /* claiming slot zero for use in class methods */
    local = &prs->currentfunccompiler->locals[0];
    prs->currentfunccompiler->localcount++;
    local->depth = 0;
    local->iscaptured = false;
    candeclthis = (
        (type != NEON_FUNCTYPE_FUNCTION) &&
        (prs->compcontext == NEON_COMPCONTEXT_CLASS)
    );
    if(candeclthis || (/*(type == NEON_FUNCTYPE_ANONYMOUS) &&*/ (prs->compcontext != NEON_COMPCONTEXT_CLASS)))
    {
        local->name.start = g_strthis;
        local->name.length = 4;
    }
    else
    {
        local->name.start = "";
        local->name.length = 0;
    }
}

int nn_astparser_makeidentconst(NNAstParser* prs, NNAstToken* name)
{
    int rawlen;
    const char* rawstr;
    NNObjString* str;
    rawstr = name->start;
    rawlen = name->length;
    if(name->isglobal)
    {
        rawstr++;
        rawlen--;
    }
    str = nn_string_copylen(prs->pvm, rawstr, rawlen);
    return nn_astparser_pushconst(prs, nn_value_fromobject(str));
}

bool nn_astparser_identsequal(NNAstToken* a, NNAstToken* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

int nn_astfunccompiler_resolvelocal(NNAstParser* prs, NNAstFuncCompiler* compiler, NNAstToken* name)
{
    int i;
    NNAstLocal* local;
    for(i = compiler->localcount - 1; i >= 0; i--)
    {
        local = &compiler->locals[i];
        if(nn_astparser_identsequal(&local->name, name))
        {
            if(local->depth == -1)
            {
                nn_astparser_raiseerror(prs, "cannot read local variable in it's own initializer");
            }
            return i;
        }
    }
    return -1;
}

int nn_astfunccompiler_addupvalue(NNAstParser* prs, NNAstFuncCompiler* compiler, uint16_t index, bool islocal)
{
    int i;
    int upcnt;
    NNAstUpvalue* upvalue;
    upcnt = compiler->targetfunc->upvalcount;
    for(i = 0; i < upcnt; i++)
    {
        upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->islocal == islocal)
        {
            return i;
        }
    }
    if(upcnt == NEON_CFG_ASTMAXUPVALS)
    {
        nn_astparser_raiseerror(prs, "too many closure variables in function");
        return 0;
    }
    compiler->upvalues[upcnt].islocal = islocal;
    compiler->upvalues[upcnt].index = index;
    return compiler->targetfunc->upvalcount++;
}

int nn_astfunccompiler_resolveupvalue(NNAstParser* prs, NNAstFuncCompiler* compiler, NNAstToken* name)
{
    int local;
    int upvalue;
    if(compiler->enclosing == NULL)
    {
        return -1;
    }
    local = nn_astfunccompiler_resolvelocal(prs, compiler->enclosing, name);
    if(local != -1)
    {
        compiler->enclosing->locals[local].iscaptured = true;
        return nn_astfunccompiler_addupvalue(prs, compiler, (uint16_t)local, true);
    }
    upvalue = nn_astfunccompiler_resolveupvalue(prs, compiler->enclosing, name);
    if(upvalue != -1)
    {
        return nn_astfunccompiler_addupvalue(prs, compiler, (uint16_t)upvalue, false);
    }
    return -1;
}

int nn_astparser_addlocal(NNAstParser* prs, NNAstToken name)
{
    NNAstLocal* local;
    if(prs->currentfunccompiler->localcount == NEON_CFG_ASTMAXLOCALS)
    {
        /* we've reached maximum local variables per scope */
        nn_astparser_raiseerror(prs, "too many local variables in scope");
        return -1;
    }
    local = &prs->currentfunccompiler->locals[prs->currentfunccompiler->localcount++];
    local->name = name;
    local->depth = -1;
    local->iscaptured = false;
    return prs->currentfunccompiler->localcount;
}

void nn_astparser_declarevariable(NNAstParser* prs)
{
    int i;
    NNAstToken* name;
    NNAstLocal* local;
    /* global variables are implicitly declared... */
    if(prs->currentfunccompiler->scopedepth == 0)
    {
        return;
    }
    name = &prs->prevtoken;
    for(i = prs->currentfunccompiler->localcount - 1; i >= 0; i--)
    {
        local = &prs->currentfunccompiler->locals[i];
        if(local->depth != -1 && local->depth < prs->currentfunccompiler->scopedepth)
        {
            break;
        }
        if(nn_astparser_identsequal(name, &local->name))
        {
            nn_astparser_raiseerror(prs, "%.*s already declared in current scope", name->length, name->start);
        }
    }
    nn_astparser_addlocal(prs, *name);
}

int nn_astparser_parsevariable(NNAstParser* prs, const char* message)
{
    if(!nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, message))
    {
        /* what to do here? */
    }
    nn_astparser_declarevariable(prs);
    /* we are in a local scope... */
    if(prs->currentfunccompiler->scopedepth > 0)
    {
        return 0;
    }
    return nn_astparser_makeidentconst(prs, &prs->prevtoken);
}

void nn_astparser_markinitialized(NNAstParser* prs)
{
    if(prs->currentfunccompiler->scopedepth == 0)
    {
        return;
    }
    prs->currentfunccompiler->locals[prs->currentfunccompiler->localcount - 1].depth = prs->currentfunccompiler->scopedepth;
}

void nn_astparser_definevariable(NNAstParser* prs, int global)
{
    /* we are in a local scope... */
    if(prs->currentfunccompiler->scopedepth > 0)
    {
        nn_astparser_markinitialized(prs);
        return;
    }
    nn_astemit_emitbyteandshort(prs, NEON_OP_GLOBALDEFINE, global);
}

NNAstToken nn_astparser_synthtoken(const char* name)
{
    NNAstToken token;
    token.isglobal = false;
    token.line = 0;
    token.type = (NNAstTokType)0;
    token.start = name;
    token.length = (int)strlen(name);
    return token;
}

NNObjFuncScript* nn_astparser_endcompiler(NNAstParser* prs)
{
    const char* fname;
    NNObjFuncScript* function;
    nn_astemit_emitreturn(prs);
    function = prs->currentfunccompiler->targetfunc;
    fname = NULL;
    if(function->name == NULL)
    {
        fname = prs->currentmodule->physicalpath->sbuf->data;
    }
    else
    {
        fname = function->name->sbuf->data;
    }
    if(!prs->haderror && prs->pvm->conf.dumpbytecode)
    {
        nn_dbg_disasmblob(prs->pvm->debugwriter, nn_astparser_currentblob(prs), fname);
    }
    NEON_ASTDEBUG(prs->pvm, "for function '%s'", fname);
    prs->currentfunccompiler = prs->currentfunccompiler->enclosing;
    return function;
}

void nn_astparser_scopebegin(NNAstParser* prs)
{
    NEON_ASTDEBUG(prs->pvm, "current depth=%d", prs->currentfunccompiler->scopedepth);
    prs->currentfunccompiler->scopedepth++;
}

bool nn_astutil_scopeendcancontinue(NNAstParser* prs)
{
    int lopos;
    int locount;
    int lodepth;
    int scodepth;
    NEON_ASTDEBUG(prs->pvm, "");
    locount = prs->currentfunccompiler->localcount;
    lopos = prs->currentfunccompiler->localcount - 1;
    lodepth = prs->currentfunccompiler->locals[lopos].depth;
    scodepth = prs->currentfunccompiler->scopedepth;
    if(locount > 0 && lodepth > scodepth)
    {
        return true;
    }
    return false;
}

void nn_astparser_scopeend(NNAstParser* prs)
{
    NEON_ASTDEBUG(prs->pvm, "current scope depth=%d", prs->currentfunccompiler->scopedepth);
    prs->currentfunccompiler->scopedepth--;
    /*
    // remove all variables declared in scope while exiting...
    */
    if(prs->keeplastvalue)
    {
        #if 0
            return;
        #endif
    }
    while(nn_astutil_scopeendcancontinue(prs))
    {
        if(prs->currentfunccompiler->locals[prs->currentfunccompiler->localcount - 1].iscaptured)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_UPVALUECLOSE);
        }
        else
        {
            nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        }
        prs->currentfunccompiler->localcount--;
    }
}

int nn_astparser_discardlocals(NNAstParser* prs, int depth)
{
    int local;
    NEON_ASTDEBUG(prs->pvm, "");
    if(prs->keeplastvalue)
    {
        #if 0
            return 0;
        #endif
    }
    if(prs->currentfunccompiler->scopedepth == -1)
    {
        nn_astparser_raiseerror(prs, "cannot exit top-level scope");
    }
    local = prs->currentfunccompiler->localcount - 1;
    while(local >= 0 && prs->currentfunccompiler->locals[local].depth >= depth)
    {
        if(prs->currentfunccompiler->locals[local].iscaptured)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_UPVALUECLOSE);
        }
        else
        {
            nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        }
        local--;
    }
    return prs->currentfunccompiler->localcount - local - 1;
}

void nn_astparser_endloop(NNAstParser* prs)
{
    int i;
    NNInstruction* bcode;
    NNValue* cvals;
    NEON_ASTDEBUG(prs->pvm, "");
    /*
    // find all NEON_OP_BREAK_PL placeholder and replace with the appropriate jump...
    */
    i = prs->innermostloopstart;
    while(i < prs->currentfunccompiler->targetfunc->blob.count)
    {
        if(prs->currentfunccompiler->targetfunc->blob.instrucs[i].code == NEON_OP_BREAK_PL)
        {
            prs->currentfunccompiler->targetfunc->blob.instrucs[i].code = NEON_OP_JUMPNOW;
            nn_astemit_patchjump(prs, i + 1);
            i += 3;
        }
        else
        {
            bcode = prs->currentfunccompiler->targetfunc->blob.instrucs;
            cvals = prs->currentfunccompiler->targetfunc->blob.constants->listitems;
            i += 1 + nn_astparser_getcodeargscount(bcode, cvals, i);
        }
    }
}

bool nn_astparser_rulebinary(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    NNAstTokType op;
    NNAstRule* rule;
    (void)previous;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    op = prs->prevtoken.type;
    /* compile the right operand */
    rule = nn_astparser_getrule(op);
    nn_astparser_parseprecedence(prs, (NNAstPrecedence)(rule->precedence + 1));
    /* emit the operator instruction */
    switch(op)
    {
        case NEON_ASTTOK_PLUS:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMADD);
            break;
        case NEON_ASTTOK_MINUS:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMSUBTRACT);
            break;
        case NEON_ASTTOK_MULTIPLY:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMMULTIPLY);
            break;
        case NEON_ASTTOK_DIVIDE:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMDIVIDE);
            break;
        case NEON_ASTTOK_MODULO:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMMODULO);
            break;
        case NEON_ASTTOK_POWEROF:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMPOW);
            break;
        case NEON_ASTTOK_FLOOR:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMFLOORDIVIDE);
            break;
            /* equality */
        case NEON_ASTTOK_EQUAL:
            nn_astemit_emitinstruc(prs, NEON_OP_EQUAL);
            break;
        case NEON_ASTTOK_NOTEQUAL:
            nn_astemit_emit2byte(prs, NEON_OP_EQUAL, NEON_OP_PRIMNOT);
            break;
        case NEON_ASTTOK_GREATERTHAN:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMGREATER);
            break;
        case NEON_ASTTOK_GREATER_EQ:
            nn_astemit_emit2byte(prs, NEON_OP_PRIMLESSTHAN, NEON_OP_PRIMNOT);
            break;
        case NEON_ASTTOK_LESSTHAN:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMLESSTHAN);
            break;
        case NEON_ASTTOK_LESSEQUAL:
            nn_astemit_emit2byte(prs, NEON_OP_PRIMGREATER, NEON_OP_PRIMNOT);
            break;
            /* bitwise */
        case NEON_ASTTOK_AMP:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMAND);
            break;
        case NEON_ASTTOK_BAR:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMOR);
            break;
        case NEON_ASTTOK_XOR:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMBITXOR);
            break;
        case NEON_ASTTOK_LEFTSHIFT:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMSHIFTLEFT);
            break;
        case NEON_ASTTOK_RIGHTSHIFT:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMSHIFTRIGHT);
            break;
            /* range */
        case NEON_ASTTOK_DOUBLEDOT:
            nn_astemit_emitinstruc(prs, NEON_OP_MAKERANGE);
            break;
        default:
            break;
    }
    return true;
}

bool nn_astparser_rulecall(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    uint8_t argcount;
    (void)previous;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    argcount = nn_astparser_parsefunccallargs(prs);
    nn_astemit_emit2byte(prs, NEON_OP_CALLFUNCTION, argcount);
    return true;
}

bool nn_astparser_ruleliteral(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    switch(prs->prevtoken.type)
    {
        case NEON_ASTTOK_KWNULL:
            nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
            break;
        case NEON_ASTTOK_KWTRUE:
            nn_astemit_emitinstruc(prs, NEON_OP_PUSHTRUE);
            break;
        case NEON_ASTTOK_KWFALSE:
            nn_astemit_emitinstruc(prs, NEON_OP_PUSHFALSE);
            break;
        default:
            /* TODO: assuming this is correct behaviour ... */
            return false;
    }
    return true;
}

void nn_astparser_parseassign(NNAstParser* prs, uint8_t realop, uint8_t getop, uint8_t setop, int arg)
{
    NEON_ASTDEBUG(prs->pvm, "");
    prs->replcanecho = false;
    if(getop == NEON_OP_PROPERTYGET || getop == NEON_OP_PROPERTYGETSELF)
    {
        nn_astemit_emitinstruc(prs, NEON_OP_DUPONE);
    }
    if(arg != -1)
    {
        nn_astemit_emitbyteandshort(prs, getop, arg);
    }
    else
    {
        nn_astemit_emit2byte(prs, getop, 1);
    }
    nn_astparser_parseexpression(prs);
    nn_astemit_emitinstruc(prs, realop);
    if(arg != -1)
    {
        nn_astemit_emitbyteandshort(prs, setop, (uint16_t)arg);
    }
    else
    {
        nn_astemit_emitinstruc(prs, setop);
    }
}

void nn_astparser_assignment(NNAstParser* prs, uint8_t getop, uint8_t setop, int arg, bool canassign)
{
    NEON_ASTDEBUG(prs->pvm, "");
    if(canassign && nn_astparser_match(prs, NEON_ASTTOK_ASSIGN))
    {
        prs->replcanecho = false;
        nn_astparser_parseexpression(prs);
        if(arg != -1)
        {
            nn_astemit_emitbyteandshort(prs, setop, (uint16_t)arg);
        }
        else
        {
            nn_astemit_emitinstruc(prs, setop);
        }
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_PLUSASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMADD, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_MINUSASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMSUBTRACT, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_MULTASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMMULTIPLY, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_DIVASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMDIVIDE, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_POWASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMPOW, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_PERCENT_EQ))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMMODULO, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_AMP_EQ))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMAND, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_BAR_EQ))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMOR, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_TILDE_EQ))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMBITNOT, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_XOR_EQ))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMBITXOR, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_LEFTSHIFTASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMSHIFTLEFT, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_RIGHTSHIFTASSIGN))
    {
        nn_astparser_parseassign(prs, NEON_OP_PRIMSHIFTRIGHT, getop, setop, arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_INCREMENT))
    {
        prs->replcanecho = false;
        if(getop == NEON_OP_PROPERTYGET || getop == NEON_OP_PROPERTYGETSELF)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_DUPONE);
        }

        if(arg != -1)
        {
            nn_astemit_emitbyteandshort(prs, getop, arg);
        }
        else
        {
            nn_astemit_emit2byte(prs, getop, 1);
        }

        nn_astemit_emit2byte(prs, NEON_OP_PUSHONE, NEON_OP_PRIMADD);
        nn_astemit_emitbyteandshort(prs, setop, (uint16_t)arg);
    }
    else if(canassign && nn_astparser_match(prs, NEON_ASTTOK_DECREMENT))
    {
        prs->replcanecho = false;
        if(getop == NEON_OP_PROPERTYGET || getop == NEON_OP_PROPERTYGETSELF)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_DUPONE);
        }

        if(arg != -1)
        {
            nn_astemit_emitbyteandshort(prs, getop, arg);
        }
        else
        {
            nn_astemit_emit2byte(prs, getop, 1);
        }

        nn_astemit_emit2byte(prs, NEON_OP_PUSHONE, NEON_OP_PRIMSUBTRACT);
        nn_astemit_emitbyteandshort(prs, setop, (uint16_t)arg);
    }
    else
    {
        if(arg != -1)
        {
            if(getop == NEON_OP_INDEXGET || getop == NEON_OP_INDEXGETRANGED)
            {
                nn_astemit_emit2byte(prs, getop, (uint8_t)0);
            }
            else
            {
                nn_astemit_emitbyteandshort(prs, getop, (uint16_t)arg);
            }
        }
        else
        {
            nn_astemit_emit2byte(prs, getop, (uint8_t)0);
        }
    }
}

bool nn_astparser_ruledot(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    int name;
    bool caninvoke;
    uint8_t argcount;
    NNOpCode getop;
    NNOpCode setop;
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_ignorewhitespace(prs);
    if(!nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "expected property name after '.'"))
    {
        return false;
    }
    name = nn_astparser_makeidentconst(prs, &prs->prevtoken);
    if(nn_astparser_match(prs, NEON_ASTTOK_PARENOPEN))
    {
        argcount = nn_astparser_parsefunccallargs(prs);
        caninvoke = (
            (prs->currentclasscompiler != NULL) &&
            (
                (previous.type == NEON_ASTTOK_KWTHIS) ||
                (nn_astparser_identsequal(&prs->prevtoken, &prs->currentclasscompiler->name))
            )
        );
        if(caninvoke)
        {
            nn_astemit_emitbyteandshort(prs, NEON_OP_CLASSINVOKETHIS, name);
        }
        else
        {
            nn_astemit_emitbyteandshort(prs, NEON_OP_CALLMETHOD, name);
        }
        nn_astemit_emit1byte(prs, argcount);
    }
    else
    {
        getop = NEON_OP_PROPERTYGET;
        setop = NEON_OP_PROPERTYSET;
        if(prs->currentclasscompiler != NULL && (previous.type == NEON_ASTTOK_KWTHIS || nn_astparser_identsequal(&prs->prevtoken, &prs->currentclasscompiler->name)))
        {
            getop = NEON_OP_PROPERTYGETSELF;
        }
        nn_astparser_assignment(prs, getop, setop, name, canassign);
    }
    return true;
}

void nn_astparser_namedvar(NNAstParser* prs, NNAstToken name, bool canassign)
{
    bool fromclass;
    uint8_t getop;
    uint8_t setop;
    int arg;
    (void)fromclass;
    NEON_ASTDEBUG(prs->pvm, " name=%.*s", name.length, name.start);
    fromclass = prs->currentclasscompiler != NULL;
    arg = nn_astfunccompiler_resolvelocal(prs, prs->currentfunccompiler, &name);
    if(arg != -1)
    {
        if(prs->infunction)
        {
            getop = NEON_OP_FUNCARGGET;
            setop = NEON_OP_FUNCARGSET;
        }
        else
        {
            getop = NEON_OP_LOCALGET;
            setop = NEON_OP_LOCALSET;
        }
    }
    else
    {
        arg = nn_astfunccompiler_resolveupvalue(prs, prs->currentfunccompiler, &name);
        if((arg != -1) && (name.isglobal == false))
        {
            getop = NEON_OP_UPVALUEGET;
            setop = NEON_OP_UPVALUESET;
        }
        else
        {
            arg = nn_astparser_makeidentconst(prs, &name);
            getop = NEON_OP_GLOBALGET;
            setop = NEON_OP_GLOBALSET;
        }
    }
    nn_astparser_assignment(prs, getop, setop, arg, canassign);
}

void nn_astparser_createdvar(NNAstParser* prs, NNAstToken name)
{
    int local;
    NEON_ASTDEBUG(prs->pvm, "name=%.*s", name.length, name.start);
    if(prs->currentfunccompiler->targetfunc->name != NULL)
    {
        local = nn_astparser_addlocal(prs, name) - 1;
        nn_astparser_markinitialized(prs);
        nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALSET, (uint16_t)local);
    }
    else
    {
        nn_astemit_emitbyteandshort(prs, NEON_OP_GLOBALDEFINE, (uint16_t)nn_astparser_makeidentconst(prs, &name));
    }
}

bool nn_astparser_rulearray(NNAstParser* prs, bool canassign)
{
    int count;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    /* placeholder for the list */
    nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
    count = 0;
    nn_astparser_ignorewhitespace(prs);
    if(!nn_astparser_check(prs, NEON_ASTTOK_BRACKETCLOSE))
    {
        do
        {
            nn_astparser_ignorewhitespace(prs);
            if(!nn_astparser_check(prs, NEON_ASTTOK_BRACKETCLOSE))
            {
                /* allow comma to end lists */
                nn_astparser_parseexpression(prs);
                nn_astparser_ignorewhitespace(prs);
                count++;
            }
            nn_astparser_ignorewhitespace(prs);
        } while(nn_astparser_match(prs, NEON_ASTTOK_COMMA));
    }
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_BRACKETCLOSE, "expected ']' at end of list");
    nn_astemit_emitbyteandshort(prs, NEON_OP_MAKEARRAY, count);
    return true;
}

bool nn_astparser_ruledictionary(NNAstParser* prs, bool canassign)
{
    bool usedexpression;
    int itemcount;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    /* placeholder for the dictionary */
    nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
    itemcount = 0;
    nn_astparser_ignorewhitespace(prs);
    if(!nn_astparser_check(prs, NEON_ASTTOK_BRACECLOSE))
    {
        do
        {
            nn_astparser_ignorewhitespace(prs);
            if(!nn_astparser_check(prs, NEON_ASTTOK_BRACECLOSE))
            {
                /* allow last pair to end with a comma */
                usedexpression = false;
                if(nn_astparser_check(prs, NEON_ASTTOK_IDENTNORMAL))
                {
                    nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "");
                    nn_astemit_emitconst(prs, nn_value_fromobject(nn_string_copylen(prs->pvm, prs->prevtoken.start, prs->prevtoken.length)));
                }
                else
                {
                    nn_astparser_parseexpression(prs);
                    usedexpression = true;
                }
                nn_astparser_ignorewhitespace(prs);
                if(!nn_astparser_check(prs, NEON_ASTTOK_COMMA) && !nn_astparser_check(prs, NEON_ASTTOK_BRACECLOSE))
                {
                    nn_astparser_consume(prs, NEON_ASTTOK_COLON, "expected ':' after dictionary key");
                    nn_astparser_ignorewhitespace(prs);

                    nn_astparser_parseexpression(prs);
                }
                else
                {
                    if(usedexpression)
                    {
                        nn_astparser_raiseerror(prs, "cannot infer dictionary values from expressions");
                        return false;
                    }
                    else
                    {
                        nn_astparser_namedvar(prs, prs->prevtoken, false);
                    }
                }
                itemcount++;
            }
        } while(nn_astparser_match(prs, NEON_ASTTOK_COMMA));
    }
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_BRACECLOSE, "expected '}' after dictionary");
    nn_astemit_emitbyteandshort(prs, NEON_OP_MAKEDICT, itemcount);
    return true;
}

bool nn_astparser_ruleindexing(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    bool assignable;
    bool commamatch;
    uint8_t getop;
    (void)previous;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    assignable = true;
    commamatch = false;
    getop = NEON_OP_INDEXGET;
    if(nn_astparser_match(prs, NEON_ASTTOK_COMMA))
    {
        nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
        commamatch = true;
        getop = NEON_OP_INDEXGETRANGED;
    }
    else
    {
        nn_astparser_parseexpression(prs);
    }
    if(!nn_astparser_match(prs, NEON_ASTTOK_BRACKETCLOSE))
    {
        getop = NEON_OP_INDEXGETRANGED;
        if(!commamatch)
        {
            nn_astparser_consume(prs, NEON_ASTTOK_COMMA, "expecting ',' or ']'");
        }
        if(nn_astparser_match(prs, NEON_ASTTOK_BRACKETCLOSE))
        {
            nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
        }
        else
        {
            nn_astparser_parseexpression(prs);
            nn_astparser_consume(prs, NEON_ASTTOK_BRACKETCLOSE, "expected ']' after indexing");
        }
        assignable = false;
    }
    else
    {
        if(commamatch)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
        }
    }
    nn_astparser_assignment(prs, getop, NEON_OP_INDEXSET, -1, assignable);
    return true;
}

bool nn_astparser_rulevarnormal(NNAstParser* prs, bool canassign)
{
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_namedvar(prs, prs->prevtoken, canassign);
    return true;
}

bool nn_astparser_rulevarglobal(NNAstParser* prs, bool canassign)
{
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_namedvar(prs, prs->prevtoken, canassign);
    return true;
}

bool nn_astparser_rulethis(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    #if 0
    if(prs->currentclasscompiler == NULL)
    {
        nn_astparser_raiseerror(prs, "cannot use keyword 'this' outside of a class");
        return false;
    }
    #endif
    #if 0
    if(prs->currentclasscompiler != NULL)
    #endif
    {
        nn_astparser_namedvar(prs, prs->prevtoken, false);
        #if 0
            nn_astparser_namedvar(prs, nn_astparser_synthtoken(g_strthis), false);
        #endif
    }
    #if 0
        nn_astemit_emitinstruc(prs, NEON_OP_CLASSGETTHIS);
    #endif
    return true;
}

bool nn_astparser_rulesuper(NNAstParser* prs, bool canassign)
{
    int name;
    bool invokeself;
    uint8_t argcount;
    NEON_ASTDEBUG(prs->pvm, "");
    (void)canassign;
    if(prs->currentclasscompiler == NULL)
    {
        nn_astparser_raiseerror(prs, "cannot use keyword 'super' outside of a class");
        return false;
    }
    else if(!prs->currentclasscompiler->hassuperclass)
    {
        nn_astparser_raiseerror(prs, "cannot use keyword 'super' in a class without a superclass");
        return false;
    }
    name = -1;
    invokeself = false;
    if(!nn_astparser_check(prs, NEON_ASTTOK_PARENOPEN))
    {
        nn_astparser_consume(prs, NEON_ASTTOK_DOT, "expected '.' or '(' after super");
        nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "expected super class method name after .");
        name = nn_astparser_makeidentconst(prs, &prs->prevtoken);
    }
    else
    {
        invokeself = true;
    }
    nn_astparser_namedvar(prs, nn_astparser_synthtoken(g_strthis), false);
    if(nn_astparser_match(prs, NEON_ASTTOK_PARENOPEN))
    {
        argcount = nn_astparser_parsefunccallargs(prs);
        nn_astparser_namedvar(prs, nn_astparser_synthtoken(g_strsuper), false);
        if(!invokeself)
        {
            nn_astemit_emitbyteandshort(prs, NEON_OP_CLASSINVOKESUPER, name);
            nn_astemit_emit1byte(prs, argcount);
        }
        else
        {
            nn_astemit_emit2byte(prs, NEON_OP_CLASSINVOKESUPERSELF, argcount);
        }
    }
    else
    {
        nn_astparser_namedvar(prs, nn_astparser_synthtoken(g_strsuper), false);
        nn_astemit_emitbyteandshort(prs, NEON_OP_CLASSGETSUPER, name);
    }
    return true;
}

bool nn_astparser_rulegrouping(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_parseexpression(prs);
    while(nn_astparser_match(prs, NEON_ASTTOK_COMMA))
    {
        nn_astparser_parseexpression(prs);
    }
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after grouped expression");
    return true;
}

NNValue nn_astparser_compilenumber(NNAstParser* prs)
{
    double dbval;
    long longval;
    int64_t llval;
    NEON_ASTDEBUG(prs->pvm, "");
    if(prs->prevtoken.type == NEON_ASTTOK_LITNUMBIN)
    {
        llval = strtoll(prs->prevtoken.start + 2, NULL, 2);
        return nn_value_makenumber(llval);
    }
    else if(prs->prevtoken.type == NEON_ASTTOK_LITNUMOCT)
    {
        longval = strtol(prs->prevtoken.start + 2, NULL, 8);
        return nn_value_makenumber(longval);
    }
    else if(prs->prevtoken.type == NEON_ASTTOK_LITNUMHEX)
    {
        longval = strtol(prs->prevtoken.start, NULL, 16);
        return nn_value_makenumber(longval);
    }
    else
    {
        dbval = strtod(prs->prevtoken.start, NULL);
        return nn_value_makenumber(dbval);
    }
}

bool nn_astparser_rulenumber(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astemit_emitconst(prs, nn_astparser_compilenumber(prs));
    return true;
}

/*
// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
*/
int nn_astparser_readhexdigit(char c)
{
    if((c >= '0') && (c <= '9'))
    {
        return (c - '0');
    }
    if((c >= 'a') && (c <= 'f'))
    {
        return ((c - 'a') + 10);
    }
    if((c >= 'A') && (c <= 'F'))
    {
        return ((c - 'A') + 10);
    }
    return -1;
}

/*
// Reads [digits] hex digits in a string literal and returns their number value.
*/
int nn_astparser_readhexescape(NNAstParser* prs, const char* str, int index, int count)
{
    size_t pos;
    int i;
    int cval;
    int digit;
    int value;
    value = 0;
    i = 0;
    digit = 0;
    for(; i < count; i++)
    {
        pos = (index + i + 2);
        cval = str[pos];
        digit = nn_astparser_readhexdigit(cval);
        if(digit == -1)
        {
            nn_astparser_raiseerror(prs, "invalid hex escape sequence at #%d of \"%s\": '%c' (%d)", pos, str, cval, cval);
        }
        value = (value * 16) | digit;
    }
    if(count == 4 && (digit = nn_astparser_readhexdigit(str[index + i + 2])) != -1)
    {
        value = (value * 16) | digit;
    }
    return value;
}

int nn_astparser_readunicodeescape(NNAstParser* prs, char* string, const char* realstring, int numberbytes, int realindex, int index)
{
    int value;
    int count;
    size_t len;
    char* chr;
    NEON_ASTDEBUG(prs->pvm, "");
    value = nn_astparser_readhexescape(prs, realstring, realindex, numberbytes);
    count = nn_util_utf8numbytes(value);
    if(count == -1)
    {
        nn_astparser_raiseerror(prs, "cannot encode a negative unicode value");
    }
    /* check for greater that \uffff */
    if(value > 65535)
    {
        count++;
    }
    if(count != 0)
    {
        chr = nn_util_utf8encode(prs->pvm, value, &len);
        if(chr)
        {
            memcpy(string + index, chr, (size_t)count + 1);
            nn_util_memfree(prs->pvm, chr);
        }
        else
        {
            nn_astparser_raiseerror(prs, "cannot decode unicode escape at index %d", realindex);
        }
    }
    /* but greater than \uffff doesn't occupy any extra byte */
    /*
    if(value > 65535)
    {
        count--;
    }
    */
    return count;
}

char* nn_astparser_compilestring(NNAstParser* prs, int* length)
{
    int k;
    int i;
    int count;
    int reallength;
    int rawlen;
    char c;
    char quote;
    char* deststr;
    char* realstr;
    rawlen = (((size_t)prs->prevtoken.length - 2) + 1);
    NEON_ASTDEBUG(prs->pvm, "raw length=%d", rawlen);
    deststr = (char*)nn_gcmem_allocate(prs->pvm, sizeof(char), rawlen);
    quote = prs->prevtoken.start[0];
    realstr = (char*)prs->prevtoken.start + 1;
    reallength = prs->prevtoken.length - 2;
    k = 0;
    for(i = 0; i < reallength; i++, k++)
    {
        c = realstr[i];
        if(c == '\\' && i < reallength - 1)
        {
            switch(realstr[i + 1])
            {
                case '0':
                    {
                        c = '\0';
                    }
                    break;
                case '$':
                    {
                        c = '$';
                    }
                    break;
                case '\'':
                    {
                        if(quote == '\'' || quote == '}')
                        {
                            /* } handle closing of interpolation. */
                            c = '\'';
                        }
                        else
                        {
                            i--;
                        }
                    }
                    break;
                case '"':
                    {
                        if(quote == '"' || quote == '}')
                        {
                            c = '"';
                        }
                        else
                        {
                            i--;
                        }
                    }
                    break;
                case 'a':
                    {
                        c = '\a';
                    }
                    break;
                case 'b':
                    {
                        c = '\b';
                    }
                    break;
                case 'f':
                    {
                        c = '\f';
                    }
                    break;
                case 'n':
                    {
                        c = '\n';
                    }
                    break;
                case 'r':
                    {
                        c = '\r';
                    }
                    break;
                case 't':
                    {
                        c = '\t';
                    }
                    break;
                case 'e':
                    {
                        c = 27;
                    }
                    break;
                case '\\':
                    {
                        c = '\\';
                    }
                    break;
                case 'v':
                    {
                        c = '\v';
                    }
                    break;
                case 'x':
                    {
                        #if 0
                            int nn_astparser_readunicodeescape(NNAstParser* prs, char* string, char* realstring, int numberbytes, int realindex, int index)
                            int nn_astparser_readhexescape(NNAstParser* prs, const char* str, int index, int count)
                            k += nn_astparser_readunicodeescape(prs, deststr, realstr, 2, i, k) - 1;
                            k += nn_astparser_readhexescape(prs, deststr, i, 2) - 0;
                        #endif
                        c = nn_astparser_readhexescape(prs, realstr, i, 2) - 0;
                        i += 2;
                        #if 0
                            continue;
                        #endif
                    }
                    break;
                case 'u':
                    {
                        count = nn_astparser_readunicodeescape(prs, deststr, realstr, 4, i, k);
                        if(count > 4)
                        {
                            k += count - 2;
                        }
                        else
                        {
                            k += count - 1;
                        }
                        if(count > 4)
                        {
                            i += 6;
                        }
                        else
                        {
                            i += 5;
                        }
                        continue;
                    }
                case 'U':
                    {
                        count = nn_astparser_readunicodeescape(prs, deststr, realstr, 8, i, k);
                        if(count > 4)
                        {
                            k += count - 2;
                        }
                        else
                        {
                            k += count - 1;
                        }
                        i += 9;
                        continue;
                    }
                default:
                    {
                        i--;
                    }
                    break;
            }
            i++;
        }
        memcpy(deststr + k, &c, 1);
    }
    *length = k;
    deststr[k] = '\0';
    return deststr;
}

bool nn_astparser_rulestring(NNAstParser* prs, bool canassign)
{
    int length;
    char* str;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "canassign=%d", canassign);
    str = nn_astparser_compilestring(prs, &length);
    nn_astemit_emitconst(prs, nn_value_fromobject(nn_string_takelen(prs->pvm, str, length)));
    return true;
}

bool nn_astparser_ruleinterpolstring(NNAstParser* prs, bool canassign)
{
    int count;
    bool doadd;
    bool stringmatched;
    NEON_ASTDEBUG(prs->pvm, "canassign=%d", canassign);
    count = 0;
    do
    {
        doadd = false;
        stringmatched = false;
        if(prs->prevtoken.length - 2 > 0)
        {
            nn_astparser_rulestring(prs, canassign);
            doadd = true;
            stringmatched = true;
            if(count > 0)
            {
                nn_astemit_emitinstruc(prs, NEON_OP_PRIMADD);
            }
        }
        nn_astparser_parseexpression(prs);
        nn_astemit_emitinstruc(prs, NEON_OP_STRINGIFY);
        if(doadd || (count >= 1 && stringmatched == false))
        {
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMADD);
        }
        count++;
    } while(nn_astparser_match(prs, NEON_ASTTOK_INTERPOLATION));
    nn_astparser_consume(prs, NEON_ASTTOK_LITERAL, "unterminated string interpolation");
    if(prs->prevtoken.length - 2 > 0)
    {
        nn_astparser_rulestring(prs, canassign);
        nn_astemit_emitinstruc(prs, NEON_OP_PRIMADD);
    }
    return true;
}

bool nn_astparser_ruleunary(NNAstParser* prs, bool canassign)
{
    NNAstTokType op;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    op = prs->prevtoken.type;
    /* compile the expression */
    nn_astparser_parseprecedence(prs, NEON_ASTPREC_UNARY);
    /* emit instruction */
    switch(op)
    {
        case NEON_ASTTOK_MINUS:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMNEGATE);
            break;
        case NEON_ASTTOK_EXCLMARK:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMNOT);
            break;
        case NEON_ASTTOK_TILDE:
            nn_astemit_emitinstruc(prs, NEON_OP_PRIMBITNOT);
            break;
        default:
            break;
    }
    return true;
}

bool nn_astparser_ruleand(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    int endjump;
    (void)previous;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    endjump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_parseprecedence(prs, NEON_ASTPREC_AND);
    nn_astemit_patchjump(prs, endjump);
    return true;
}

bool nn_astparser_ruleor(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    int endjump;
    int elsejump;
    (void)previous;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    elsejump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    endjump = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
    nn_astemit_patchjump(prs, elsejump);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_parseprecedence(prs, NEON_ASTPREC_OR);
    nn_astemit_patchjump(prs, endjump);
    return true;
}

bool nn_astparser_ruleconditional(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    int thenjump;
    int elsejump;
    (void)previous;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    thenjump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_ignorewhitespace(prs);
    /* compile the then expression */
    nn_astparser_parseprecedence(prs, NEON_ASTPREC_CONDITIONAL);
    nn_astparser_ignorewhitespace(prs);
    elsejump = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
    nn_astemit_patchjump(prs, thenjump);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_consume(prs, NEON_ASTTOK_COLON, "expected matching ':' after '?' conditional");
    nn_astparser_ignorewhitespace(prs);
    /*
    // compile the else expression
    // here we parse at NEON_ASTPREC_ASSIGNMENT precedence as
    // linear conditionals can be nested.
    */
    nn_astparser_parseprecedence(prs, NEON_ASTPREC_ASSIGNMENT);
    nn_astemit_patchjump(prs, elsejump);
    return true;
}

bool nn_astparser_ruleimport(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_parseexpression(prs);
    nn_astemit_emitinstruc(prs, NEON_OP_IMPORTIMPORT);
    return true;
}

bool nn_astparser_rulenew(NNAstParser* prs, bool canassign)
{
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "class name after 'new'");
    return nn_astparser_rulevarnormal(prs, canassign);
}

bool nn_astparser_ruletypeof(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' after 'typeof'");
    nn_astparser_parseexpression(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after 'typeof'");
    nn_astemit_emitinstruc(prs, NEON_OP_TYPEOF);
    return true;
}

bool nn_astparser_rulenothingprefix(NNAstParser* prs, bool canassign)
{
    (void)prs;
    (void)canassign;
    NEON_ASTDEBUG(prs->pvm, "");
    return true;
}

bool nn_astparser_rulenothinginfix(NNAstParser* prs, NNAstToken previous, bool canassign)
{
    (void)prs;
    (void)previous;
    (void)canassign;
    return true;
}

NNAstRule* nn_astparser_putrule(NNAstRule* dest, NNAstParsePrefixFN prefix, NNAstParseInfixFN infix, NNAstPrecedence precedence)
{
    dest->prefix = prefix;
    dest->infix = infix;
    dest->precedence = precedence;
    return dest;
}

#define dorule(tok, prefix, infix, precedence) \
    case tok: return nn_astparser_putrule(&dest, prefix, infix, precedence);

NNAstRule* nn_astparser_getrule(NNAstTokType type)
{
    static NNAstRule dest;
    switch(type)
    {
        dorule(NEON_ASTTOK_NEWLINE, nn_astparser_rulenothingprefix, nn_astparser_rulenothinginfix, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_PARENOPEN, nn_astparser_rulegrouping, nn_astparser_rulecall, NEON_ASTPREC_CALL );
        dorule(NEON_ASTTOK_PARENCLOSE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_BRACKETOPEN, nn_astparser_rulearray, nn_astparser_ruleindexing, NEON_ASTPREC_CALL );
        dorule(NEON_ASTTOK_BRACKETCLOSE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_BRACEOPEN, nn_astparser_ruledictionary, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_BRACECLOSE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_SEMICOLON, nn_astparser_rulenothingprefix, nn_astparser_rulenothinginfix, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_COMMA, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_BACKSLASH, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_EXCLMARK, nn_astparser_ruleunary, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_NOTEQUAL, NULL, nn_astparser_rulebinary, NEON_ASTPREC_EQUALITY );
        dorule(NEON_ASTTOK_COLON, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_AT, nn_astparser_ruleanonfunc, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_DOT, NULL, nn_astparser_ruledot, NEON_ASTPREC_CALL );
        dorule(NEON_ASTTOK_DOUBLEDOT, NULL, nn_astparser_rulebinary, NEON_ASTPREC_RANGE );
        dorule(NEON_ASTTOK_TRIPLEDOT, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_PLUS, nn_astparser_ruleunary, nn_astparser_rulebinary, NEON_ASTPREC_TERM );
        dorule(NEON_ASTTOK_PLUSASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_INCREMENT, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_MINUS, nn_astparser_ruleunary, nn_astparser_rulebinary, NEON_ASTPREC_TERM );
        dorule(NEON_ASTTOK_MINUSASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_DECREMENT, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_MULTIPLY, NULL, nn_astparser_rulebinary, NEON_ASTPREC_FACTOR );
        dorule(NEON_ASTTOK_MULTASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_POWEROF, NULL, nn_astparser_rulebinary, NEON_ASTPREC_FACTOR );
        dorule(NEON_ASTTOK_POWASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_DIVIDE, NULL, nn_astparser_rulebinary, NEON_ASTPREC_FACTOR );
        dorule(NEON_ASTTOK_DIVASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_FLOOR, NULL, nn_astparser_rulebinary, NEON_ASTPREC_FACTOR );
        dorule(NEON_ASTTOK_ASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_EQUAL, NULL, nn_astparser_rulebinary, NEON_ASTPREC_EQUALITY );
        dorule(NEON_ASTTOK_LESSTHAN, NULL, nn_astparser_rulebinary, NEON_ASTPREC_COMPARISON );
        dorule(NEON_ASTTOK_LESSEQUAL, NULL, nn_astparser_rulebinary, NEON_ASTPREC_COMPARISON );
        dorule(NEON_ASTTOK_LEFTSHIFT, NULL, nn_astparser_rulebinary, NEON_ASTPREC_SHIFT );
        dorule(NEON_ASTTOK_LEFTSHIFTASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_GREATERTHAN, NULL, nn_astparser_rulebinary, NEON_ASTPREC_COMPARISON );
        dorule(NEON_ASTTOK_GREATER_EQ, NULL, nn_astparser_rulebinary, NEON_ASTPREC_COMPARISON );
        dorule(NEON_ASTTOK_RIGHTSHIFT, NULL, nn_astparser_rulebinary, NEON_ASTPREC_SHIFT );
        dorule(NEON_ASTTOK_RIGHTSHIFTASSIGN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_MODULO, NULL, nn_astparser_rulebinary, NEON_ASTPREC_FACTOR );
        dorule(NEON_ASTTOK_PERCENT_EQ, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_AMP, NULL, nn_astparser_rulebinary, NEON_ASTPREC_BITAND );
        dorule(NEON_ASTTOK_AMP_EQ, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_BAR, /*nn_astparser_ruleanoncompat*/ NULL, nn_astparser_rulebinary, NEON_ASTPREC_BITOR );
        dorule(NEON_ASTTOK_BAR_EQ, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_TILDE, nn_astparser_ruleunary, NULL, NEON_ASTPREC_UNARY );
        dorule(NEON_ASTTOK_TILDE_EQ, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_XOR, NULL, nn_astparser_rulebinary, NEON_ASTPREC_BITXOR );
        dorule(NEON_ASTTOK_XOR_EQ, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_QUESTION, NULL, nn_astparser_ruleconditional, NEON_ASTPREC_CONDITIONAL );
        dorule(NEON_ASTTOK_KWAND, NULL, nn_astparser_ruleand, NEON_ASTPREC_AND );
        dorule(NEON_ASTTOK_KWAS, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWASSERT, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWBREAK, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWCLASS, nn_astparser_ruleanonclass, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWCONTINUE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWFUNCTION, nn_astparser_ruleanonfunc, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWDEFAULT, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWTHROW, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWDO, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWECHO, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWELSE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWFALSE, nn_astparser_ruleliteral, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWFOREACH, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWIF, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWIMPORT, nn_astparser_ruleimport, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWIN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWFOR, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWVAR, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWNULL, nn_astparser_ruleliteral, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWNEW, nn_astparser_rulenew, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWTYPEOF, nn_astparser_ruletypeof, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWOR, NULL, nn_astparser_ruleor, NEON_ASTPREC_OR );
        dorule(NEON_ASTTOK_KWSUPER, nn_astparser_rulesuper, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWRETURN, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWTHIS, nn_astparser_rulethis, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWSTATIC, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWTRUE, nn_astparser_ruleliteral, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWSWITCH, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWCASE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWWHILE, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWTRY, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWCATCH, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWFINALLY, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_LITERAL, nn_astparser_rulestring, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_LITNUMREG, nn_astparser_rulenumber, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_LITNUMBIN, nn_astparser_rulenumber, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_LITNUMOCT, nn_astparser_rulenumber, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_LITNUMHEX, nn_astparser_rulenumber, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_IDENTNORMAL, nn_astparser_rulevarnormal, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_INTERPOLATION, nn_astparser_ruleinterpolstring, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_EOF, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_ERROR, NULL, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_KWEMPTY, nn_astparser_ruleliteral, NULL, NEON_ASTPREC_NONE );
        dorule(NEON_ASTTOK_UNDEFINED, NULL, NULL, NEON_ASTPREC_NONE );
        default:
            fprintf(stderr, "missing rule?\n");
            break;
    }
    return NULL;
}
#undef dorule

bool nn_astparser_doparseprecedence(NNAstParser* prs, NNAstPrecedence precedence/*, NNAstExpression* dest*/)
{
    bool canassign;
    NNAstToken previous;
    NNAstParseInfixFN infixrule;
    NNAstParsePrefixFN prefixrule;
    prefixrule = nn_astparser_getrule(prs->prevtoken.type)->prefix;
    if(prefixrule == NULL)
    {
        nn_astparser_raiseerror(prs, "expected expression");
        return false;
    }
    canassign = precedence <= NEON_ASTPREC_ASSIGNMENT;
    prefixrule(prs, canassign);
    while(precedence <= nn_astparser_getrule(prs->currtoken.type)->precedence)
    {
        previous = prs->prevtoken;
        nn_astparser_ignorewhitespace(prs);
        nn_astparser_advance(prs);
        infixrule = nn_astparser_getrule(prs->prevtoken.type)->infix;
        infixrule(prs, previous, canassign);
    }
    if(canassign && nn_astparser_match(prs, NEON_ASTTOK_ASSIGN))
    {
        nn_astparser_raiseerror(prs, "invalid assignment target");
        return false;
    }
    return true;
}

bool nn_astparser_parseprecedence(NNAstParser* prs, NNAstPrecedence precedence)
{
    if(nn_astlex_isatend(prs->lexer) && prs->pvm->isrepl)
    {
        return false;
    }
    nn_astparser_ignorewhitespace(prs);
    if(nn_astlex_isatend(prs->lexer) && prs->pvm->isrepl)
    {
        return false;
    }
    nn_astparser_advance(prs);
    return nn_astparser_doparseprecedence(prs, precedence);
}

bool nn_astparser_parseprecnoadvance(NNAstParser* prs, NNAstPrecedence precedence)
{
    if(nn_astlex_isatend(prs->lexer) && prs->pvm->isrepl)
    {
        return false;
    }
    nn_astparser_ignorewhitespace(prs);
    if(nn_astlex_isatend(prs->lexer) && prs->pvm->isrepl)
    {
        return false;
    }
    return nn_astparser_doparseprecedence(prs, precedence);
}

bool nn_astparser_parseexpression(NNAstParser* prs)
{
    return nn_astparser_parseprecedence(prs, NEON_ASTPREC_ASSIGNMENT);
}

bool nn_astparser_parseblock(NNAstParser* prs)
{
    prs->blockcount++;
    nn_astparser_ignorewhitespace(prs);
    while(!nn_astparser_check(prs, NEON_ASTTOK_BRACECLOSE) && !nn_astparser_check(prs, NEON_ASTTOK_EOF))
    {
        nn_astparser_parsedeclaration(prs);
    }
    prs->blockcount--;
    if(!nn_astparser_consume(prs, NEON_ASTTOK_BRACECLOSE, "expected '}' after block"))
    {
        return false;
    }
    if(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON))
    {
    }
    return true;
}

void nn_astparser_declarefuncargvar(NNAstParser* prs)
{
    int i;
    NNAstToken* name;
    NNAstLocal* local;
    /* global variables are implicitly declared... */
    if(prs->currentfunccompiler->scopedepth == 0)
    {
        return;
    }
    name = &prs->prevtoken;
    for(i = prs->currentfunccompiler->localcount - 1; i >= 0; i--)
    {
        local = &prs->currentfunccompiler->locals[i];
        if(local->depth != -1 && local->depth < prs->currentfunccompiler->scopedepth)
        {
            break;
        }
        if(nn_astparser_identsequal(name, &local->name))
        {
            nn_astparser_raiseerror(prs, "%.*s already declared in current scope", name->length, name->start);
        }
    }
    nn_astparser_addlocal(prs, *name);
}


int nn_astparser_parsefuncparamvar(NNAstParser* prs, const char* message)
{
    if(!nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, message))
    {
        /* what to do here? */
    }
    nn_astparser_declarefuncargvar(prs);
    /* we are in a local scope... */
    if(prs->currentfunccompiler->scopedepth > 0)
    {
        return 0;
    }
    return nn_astparser_makeidentconst(prs, &prs->prevtoken);
}

uint8_t nn_astparser_parsefunccallargs(NNAstParser* prs)
{
    uint8_t argcount;
    argcount = 0;
    if(!nn_astparser_check(prs, NEON_ASTTOK_PARENCLOSE))
    {
        do
        {
            nn_astparser_ignorewhitespace(prs);
            nn_astparser_parseexpression(prs);
            if(argcount == NEON_CFG_ASTMAXFUNCPARAMS)
            {
                nn_astparser_raiseerror(prs, "cannot have more than %d arguments to a function", NEON_CFG_ASTMAXFUNCPARAMS);
            }
            argcount++;
        } while(nn_astparser_match(prs, NEON_ASTTOK_COMMA));
    }
    nn_astparser_ignorewhitespace(prs);
    if(!nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after argument list"))
    {
        /* TODO: handle this, somehow. */
    }
    return argcount;
}

void nn_astparser_parsefuncparamlist(NNAstParser* prs)
{
    int paramconstant;
    /* compile argument list... */
    do
    {
        nn_astparser_ignorewhitespace(prs);
        prs->currentfunccompiler->targetfunc->arity++;
        #if 0
        if(prs->currentfunccompiler->targetfunc->arity > NEON_CFG_ASTMAXFUNCPARAMS)
        {
            nn_astparser_raiseerroratcurrent(prs, "cannot have more than %d function parameters", NEON_CFG_ASTMAXFUNCPARAMS);
        }
        #endif
        if(nn_astparser_match(prs, NEON_ASTTOK_TRIPLEDOT))
        {
            prs->currentfunccompiler->targetfunc->isvariadic = true;
            nn_astparser_addlocal(prs, nn_astparser_synthtoken("__args__"));
            nn_astparser_definevariable(prs, 0);
            break;
        }
        paramconstant = nn_astparser_parsefuncparamvar(prs, "expected parameter name");
        nn_astparser_definevariable(prs, paramconstant);
        nn_astparser_ignorewhitespace(prs);
    } while(nn_astparser_match(prs, NEON_ASTTOK_COMMA));
}

void nn_astfunccompiler_compilebody(NNAstParser* prs, NNAstFuncCompiler* compiler, bool closescope, bool isanon)
{
    int i;
    NNObjFuncScript* function;
    (void)isanon;
    /* compile the body */
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_BRACEOPEN, "expected '{' before function body");
    nn_astparser_parseblock(prs);
    /* create the function object */
    if(closescope)
    {
        nn_astparser_scopeend(prs);
    }
    function = nn_astparser_endcompiler(prs);
    nn_vm_stackpush(prs->pvm, nn_value_fromobject(function));
    nn_astemit_emitbyteandshort(prs, NEON_OP_MAKECLOSURE, nn_astparser_pushconst(prs, nn_value_fromobject(function)));
    for(i = 0; i < function->upvalcount; i++)
    {
        nn_astemit_emit1byte(prs, compiler->upvalues[i].islocal ? 1 : 0);
        nn_astemit_emit1short(prs, compiler->upvalues[i].index);
    }
    nn_vm_stackpop(prs->pvm);
}

void nn_astparser_parsefuncfull(NNAstParser* prs, NNFuncType type, bool isanon)
{
    NNAstFuncCompiler compiler;
    prs->infunction = true;
    nn_astfunccompiler_init(prs, &compiler, type, isanon);
    nn_astparser_scopebegin(prs);
    /* compile parameter list */
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' after function name");
    if(!nn_astparser_check(prs, NEON_ASTTOK_PARENCLOSE))
    {
        nn_astparser_parsefuncparamlist(prs);
    }
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after function parameters");
    nn_astfunccompiler_compilebody(prs, &compiler, false, isanon);
    prs->infunction = false;
}

void nn_astparser_parsemethod(NNAstParser* prs, NNAstToken classname, bool isstatic)
{
    size_t sn;
    int constant;
    const char* sc;
    NNFuncType type;
    static NNAstTokType tkns[] = { NEON_ASTTOK_IDENTNORMAL, NEON_ASTTOK_DECORATOR };
    (void)classname;
    (void)isstatic;
    sc = "constructor";
    sn = strlen(sc);
    nn_astparser_consumeor(prs, "method name expected", tkns, 2);
    constant = nn_astparser_makeidentconst(prs, &prs->prevtoken);
    type = NEON_FUNCTYPE_METHOD;
    if((prs->prevtoken.length == (int)sn) && (memcmp(prs->prevtoken.start, sc, sn) == 0))
    {
        type = NEON_FUNCTYPE_INITIALIZER;
    }
    else if((prs->prevtoken.length > 0) && (prs->prevtoken.start[0] == '_'))
    {
        type = NEON_FUNCTYPE_PRIVATE;
    }
    nn_astparser_parsefuncfull(prs, type, false);
    nn_astemit_emitbyteandshort(prs, NEON_OP_MAKEMETHOD, constant);
}

bool nn_astparser_ruleanonfunc(NNAstParser* prs, bool canassign)
{
    NNAstFuncCompiler compiler;
    (void)canassign;
    nn_astfunccompiler_init(prs, &compiler, NEON_FUNCTYPE_FUNCTION, true);
    nn_astparser_scopebegin(prs);
    /* compile parameter list */
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' at start of anonymous function");
    if(!nn_astparser_check(prs, NEON_ASTTOK_PARENCLOSE))
    {
        nn_astparser_parsefuncparamlist(prs);
    }
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after anonymous function parameters");
    nn_astfunccompiler_compilebody(prs, &compiler, true, true);
    return true;
}


bool nn_astparser_ruleanonclass(NNAstParser* prs, bool canassign)
{
    (void)canassign;
    nn_astparser_parseclassdeclaration(prs, false);
    return true;
}

void nn_astparser_parsefield(NNAstParser* prs, bool isstatic)
{
    int fieldconstant;
    nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "class property name expected");
    fieldconstant = nn_astparser_makeidentconst(prs, &prs->prevtoken);
    if(nn_astparser_match(prs, NEON_ASTTOK_ASSIGN))
    {
        nn_astparser_parseexpression(prs);
    }
    else
    {
        nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
    }
    nn_astemit_emitbyteandshort(prs, NEON_OP_CLASSPROPERTYDEFINE, fieldconstant);
    nn_astemit_emit1byte(prs, isstatic ? 1 : 0);
    nn_astparser_consumestmtend(prs);
    nn_astparser_ignorewhitespace(prs);
}

void nn_astparser_parsefuncdecl(NNAstParser* prs)
{
    int global;
    global = nn_astparser_parsevariable(prs, "function name expected");
    nn_astparser_markinitialized(prs);
    nn_astparser_parsefuncfull(prs, NEON_FUNCTYPE_FUNCTION, false);
    nn_astparser_definevariable(prs, global);
}

void nn_astparser_parseclassdeclaration(NNAstParser* prs, bool named)
{
    bool isstatic;
    int nameconst;
    NNAstCompContext oldctx;
    NNAstToken classname;
    NNAstClassCompiler classcompiler;
    /*
                ClassCompiler classcompiler;
                classcompiler.hasname = named;
                if(named)
                {
                    consume(Token::TOK_IDENTNORMAL, "class name expected");
                    classname = m_prevtoken;
                    declareVariable();
                }
                else
                {
                    classname = makeSynthToken("<anonclass>");
                }
                nameconst = makeIdentConst(&classname);
    */
    if(named)
    {
        nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "class name expected");
        classname = prs->prevtoken;
        nn_astparser_declarevariable(prs);
    }
    else
    {
        classname = nn_astparser_synthtoken("<anonclass>");
    }
    nameconst = nn_astparser_makeidentconst(prs, &classname);
    nn_astemit_emitbyteandshort(prs, NEON_OP_MAKECLASS, nameconst);
    nn_astparser_definevariable(prs, nameconst);
    classcompiler.name = prs->prevtoken;
    classcompiler.hassuperclass = false;
    classcompiler.enclosing = prs->currentclasscompiler;
    prs->currentclasscompiler = &classcompiler;
    oldctx = prs->compcontext;
    prs->compcontext = NEON_COMPCONTEXT_CLASS;
    if(nn_astparser_match(prs, NEON_ASTTOK_LESSTHAN))
    {
        nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "name of superclass expected");
        nn_astparser_rulevarnormal(prs, false);
        if(nn_astparser_identsequal(&classname, &prs->prevtoken))
        {
            nn_astparser_raiseerror(prs, "class %.*s cannot inherit from itself", classname.length, classname.start);
        }
        nn_astparser_scopebegin(prs);
        nn_astparser_addlocal(prs, nn_astparser_synthtoken(g_strsuper));
        nn_astparser_definevariable(prs, 0);
        nn_astparser_namedvar(prs, classname, false);
        nn_astemit_emitinstruc(prs, NEON_OP_CLASSINHERIT);
        classcompiler.hassuperclass = true;
    }
    nn_astparser_namedvar(prs, classname, false);
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_BRACEOPEN, "expected '{' before class body");
    nn_astparser_ignorewhitespace(prs);
    while(!nn_astparser_check(prs, NEON_ASTTOK_BRACECLOSE) && !nn_astparser_check(prs, NEON_ASTTOK_EOF))
    {
        isstatic = false;
        if(nn_astparser_match(prs, NEON_ASTTOK_KWSTATIC))
        {
            isstatic = true;
        }

        if(nn_astparser_match(prs, NEON_ASTTOK_KWVAR))
        {
            nn_astparser_parsefield(prs, isstatic);
        }
        else
        {
            nn_astparser_parsemethod(prs, classname, isstatic);
            nn_astparser_ignorewhitespace(prs);
        }
    }
    nn_astparser_consume(prs, NEON_ASTTOK_BRACECLOSE, "expected '}' after class body");
    if(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON))
    {
    }
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    if(classcompiler.hassuperclass)
    {
        nn_astparser_scopeend(prs);
    }
    prs->currentclasscompiler = prs->currentclasscompiler->enclosing;
    prs->compcontext = oldctx;
}

void nn_astparser_parsevardecl(NNAstParser* prs, bool isinitializer)
{
    int global;
    int totalparsed;
    totalparsed = 0;
    do
    {
        if(totalparsed > 0)
        {
            nn_astparser_ignorewhitespace(prs);
        }
        global = nn_astparser_parsevariable(prs, "variable name expected");
        if(nn_astparser_match(prs, NEON_ASTTOK_ASSIGN))
        {
            nn_astparser_parseexpression(prs);
        }
        else
        {
            nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
        }
        nn_astparser_definevariable(prs, global);
        totalparsed++;
    } while(nn_astparser_match(prs, NEON_ASTTOK_COMMA));

    if(!isinitializer)
    {
        nn_astparser_consumestmtend(prs);
    }
    else
    {
        nn_astparser_consume(prs, NEON_ASTTOK_SEMICOLON, "expected ';' after initializer");
        nn_astparser_ignorewhitespace(prs);
    }
}

void nn_astparser_parseexprstmt(NNAstParser* prs, bool isinitializer, bool semi)
{
    if(prs->pvm->isrepl && prs->currentfunccompiler->scopedepth == 0)
    {
        prs->replcanecho = true;
    }
    if(!semi)
    {
        nn_astparser_parseexpression(prs);
    }
    else
    {
        nn_astparser_parseprecnoadvance(prs, NEON_ASTPREC_ASSIGNMENT);
    }
    if(!isinitializer)
    {
        if(prs->replcanecho && prs->pvm->isrepl)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_ECHO);
            prs->replcanecho = false;
        }
        else
        {
            #if 0
            if(!prs->keeplastvalue)
            #endif
            {
                nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
            }
        }
        nn_astparser_consumestmtend(prs);
    }
    else
    {
        nn_astparser_consume(prs, NEON_ASTTOK_SEMICOLON, "expected ';' after initializer");
        nn_astparser_ignorewhitespace(prs);
        nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    }
}

/**
 * iter statements are like for loops in c...
 * they are desugared into a while loop
 *
 * i.e.
 *
 * iter i = 0; i < 10; i++ {
 *    ...
 * }
 *
 * desugars into:
 *
 * var i = 0
 * while i < 10 {
 *    ...
 *    i = i + 1
 * }
 */
void nn_astparser_parseforstmt(NNAstParser* prs)
{
    int exitjump;
    int bodyjump;
    int incrstart;
    int surroundingloopstart;
    int surroundingscopedepth;
    nn_astparser_scopebegin(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' after 'for'");
    /* parse initializer... */
    if(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON))
    {
        /* no initializer */
    }
    else if(nn_astparser_match(prs, NEON_ASTTOK_KWVAR))
    {
        nn_astparser_parsevardecl(prs, true);
    }
    else
    {
        nn_astparser_parseexprstmt(prs, true, false);
    }
    /* keep a copy of the surrounding loop's start and depth */
    surroundingloopstart = prs->innermostloopstart;
    surroundingscopedepth = prs->innermostloopscopedepth;
    /* update the parser's loop start and depth to the current */
    prs->innermostloopstart = nn_astparser_currentblob(prs)->count;
    prs->innermostloopscopedepth = prs->currentfunccompiler->scopedepth;
    exitjump = -1;
    if(!nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON))
    {
        /* the condition is optional */
        nn_astparser_parseexpression(prs);
        nn_astparser_consume(prs, NEON_ASTTOK_SEMICOLON, "expected ';' after condition");
        nn_astparser_ignorewhitespace(prs);
        /* jump out of the loop if the condition is false... */
        exitjump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
        nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        /* pop the condition */
    }
    /* the iterator... */
    if(!nn_astparser_check(prs, NEON_ASTTOK_BRACEOPEN))
    {
        bodyjump = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
        incrstart = nn_astparser_currentblob(prs)->count;
        nn_astparser_parseexpression(prs);
        nn_astparser_ignorewhitespace(prs);
        nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        nn_astemit_emitloop(prs, prs->innermostloopstart);
        prs->innermostloopstart = incrstart;
        nn_astemit_patchjump(prs, bodyjump);
    }
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after 'for'");
    nn_astparser_parsestmt(prs);
    nn_astemit_emitloop(prs, prs->innermostloopstart);
    if(exitjump != -1)
    {
        nn_astemit_patchjump(prs, exitjump);
        nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    }
    nn_astparser_endloop(prs);
    /* reset the loop start and scope depth to the surrounding value */
    prs->innermostloopstart = surroundingloopstart;
    prs->innermostloopscopedepth = surroundingscopedepth;
    nn_astparser_scopeend(prs);
}

/**
 * for x in iterable {
 *    ...
 * }
 *
 * ==
 *
 * {
 *    var iterable = expression()
 *    var _
 *
 *    while _ = iterable.@itern() {
 *      var x = iterable.@iter()
 *      ...
 *    }
 * }
 *
 * ---------------------------------
 *
 * foreach x, y in iterable {
 *    ...
 * }
 *
 * ==
 *
 * {
 *    var iterable = expression()
 *    var x
 *
 *    while x = iterable.@itern() {
 *      var y = iterable.@iter()
 *      ...
 *    }
 * }
 *
 * Every iterable Object must implement the @iter(x) and the @itern(x)
 * function.
 *
 * to make instances of a user created class iterable,
 * the class must implement the @iter(x) and the @itern(x) function.
 * the @itern(x) must return the current iterating index of the object and
 * the
 * @iter(x) function must return the value at that index.
 * _NOTE_: the @iter(x) function will no longer be called after the
 * @itern(x) function returns a false value. so the @iter(x) never needs
 * to return a false value
 */
void nn_astparser_parseforeachstmt(NNAstParser* prs)
{
    int citer;
    int citern;
    int falsejump;
    int keyslot;
    int valueslot;
    int iteratorslot;
    int surroundingloopstart;
    int surroundingscopedepth;
    NNAstToken iteratortoken;
    NNAstToken keytoken;
    NNAstToken valuetoken;
    nn_astparser_scopebegin(prs);
    /* define @iter and @itern constant */
    citer = nn_astparser_pushconst(prs, nn_value_fromobject(nn_string_copycstr(prs->pvm, "@iter")));
    citern = nn_astparser_pushconst(prs, nn_value_fromobject(nn_string_copycstr(prs->pvm, "@itern")));
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' after 'foreach'");
    nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "expected variable name after 'foreach'");
    if(!nn_astparser_check(prs, NEON_ASTTOK_COMMA))
    {
        keytoken = nn_astparser_synthtoken(" _ ");
        valuetoken = prs->prevtoken;
    }
    else
    {
        keytoken = prs->prevtoken;
        nn_astparser_consume(prs, NEON_ASTTOK_COMMA, "");
        nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "expected variable name after ','");
        valuetoken = prs->prevtoken;
    }
    nn_astparser_consume(prs, NEON_ASTTOK_KWIN, "expected 'in' after for loop variable(s)");
    nn_astparser_ignorewhitespace(prs);
    /*
    // The space in the variable name ensures it won't collide with a user-defined
    // variable.
    */
    iteratortoken = nn_astparser_synthtoken(" iterator ");
    /* Evaluate the sequence expression and store it in a hidden local variable. */
    nn_astparser_parseexpression(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after 'foreach'");
    if(prs->currentfunccompiler->localcount + 3 > NEON_CFG_ASTMAXLOCALS)
    {
        nn_astparser_raiseerror(prs, "cannot declare more than %d variables in one scope", NEON_CFG_ASTMAXLOCALS);
        return;
    }
    /* add the iterator to the local scope */
    iteratorslot = nn_astparser_addlocal(prs, iteratortoken) - 1;
    nn_astparser_definevariable(prs, 0);
    /* Create the key local variable. */
    nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
    keyslot = nn_astparser_addlocal(prs, keytoken) - 1;
    nn_astparser_definevariable(prs, keyslot);
    /* create the local value slot */
    nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
    valueslot = nn_astparser_addlocal(prs, valuetoken) - 1;
    nn_astparser_definevariable(prs, 0);
    surroundingloopstart = prs->innermostloopstart;
    surroundingscopedepth = prs->innermostloopscopedepth;
    /*
    // we'll be jumping back to right before the
    // expression after the loop body
    */
    prs->innermostloopstart = nn_astparser_currentblob(prs)->count;
    prs->innermostloopscopedepth = prs->currentfunccompiler->scopedepth;
    /* key = iterable.iter_n__(key) */
    nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALGET, iteratorslot);
    nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALGET, keyslot);
    nn_astemit_emitbyteandshort(prs, NEON_OP_CALLMETHOD, citern);
    nn_astemit_emit1byte(prs, 1);
    nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALSET, keyslot);
    falsejump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    /* value = iterable.iter__(key) */
    nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALGET, iteratorslot);
    nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALGET, keyslot);
    nn_astemit_emitbyteandshort(prs, NEON_OP_CALLMETHOD, citer);
    nn_astemit_emit1byte(prs, 1);
    /*
    // Bind the loop value in its own scope. This ensures we get a fresh
    // variable each iteration so that closures for it don't all see the same one.
    */
    nn_astparser_scopebegin(prs);
    /* update the value */
    nn_astemit_emitbyteandshort(prs, NEON_OP_LOCALSET, valueslot);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_parsestmt(prs);
    nn_astparser_scopeend(prs);
    nn_astemit_emitloop(prs, prs->innermostloopstart);
    nn_astemit_patchjump(prs, falsejump);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_endloop(prs);
    prs->innermostloopstart = surroundingloopstart;
    prs->innermostloopscopedepth = surroundingscopedepth;
    nn_astparser_scopeend(prs);
}

/**
 * switch expression {
 *    case expression {
 *      ...
 *    }
 *    case expression {
 *      ...
 *    }
 *    ...
 * }
 */
void nn_astparser_parseswitchstmt(NNAstParser* prs)
{
    int i;
    int length;
    int swstate;
    int casecount;
    int switchcode;
    int startoffset;
    int caseends[NEON_CFG_ASTMAXSWITCHCASES];
    char* str;
    NNValue jump;
    NNAstTokType casetype;
    NNObjSwitch* sw;
    NNObjString* string;
    /* the expression */
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' before 'switch'");

    nn_astparser_parseexpression(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after 'switch'");
    nn_astparser_ignorewhitespace(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_BRACEOPEN, "expected '{' after 'switch' expression");
    nn_astparser_ignorewhitespace(prs);

    /*
                consume(Token::TOK_PARENOPEN, "expected '(' 'switch'");
                parseExpression();
                consume(Token::TOK_PARENCLOSE, "expected ')' 'switch' expression");
                ignoreSpace();
                consume(Token::TOK_BRACEOPEN, "expected '{' after 'switch' expression");
                ignoreSpace();
    */

    /* 0: before all cases, 1: before default, 2: after default */
    swstate = 0;
    casecount = 0;
    sw = nn_object_makeswitch(prs->pvm);
    nn_vm_stackpush(prs->pvm, nn_value_fromobject(sw));
    switchcode = nn_astemit_emitswitch(prs);
    /* nn_astemit_emitbyteandshort(prs, NEON_OP_SWITCH, nn_astparser_pushconst(prs, nn_value_fromobject(sw))); */
    startoffset = nn_astparser_currentblob(prs)->count;
    while(!nn_astparser_match(prs, NEON_ASTTOK_BRACECLOSE) && !nn_astparser_check(prs, NEON_ASTTOK_EOF))
    {
        if(nn_astparser_match(prs, NEON_ASTTOK_KWCASE) || nn_astparser_match(prs, NEON_ASTTOK_KWDEFAULT))
        {
            casetype = prs->prevtoken.type;
            if(swstate == 2)
            {
                nn_astparser_raiseerror(prs, "cannot have another case after a default case");
            }
            if(swstate == 1)
            {
                /* at the end of the previous case, jump over the others... */
                caseends[casecount++] = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
            }
            if(casetype == NEON_ASTTOK_KWCASE)
            {
                swstate = 1;
                do
                {
                    nn_astparser_ignorewhitespace(prs);
                    nn_astparser_advance(prs);
                    jump = nn_value_makenumber((double)nn_astparser_currentblob(prs)->count - (double)startoffset);
                    if(prs->prevtoken.type == NEON_ASTTOK_KWTRUE)
                    {
                        nn_table_set(sw->table, nn_value_makebool(true), jump);
                    }
                    else if(prs->prevtoken.type == NEON_ASTTOK_KWFALSE)
                    {
                        nn_table_set(sw->table, nn_value_makebool(false), jump);
                    }
                    else if(prs->prevtoken.type == NEON_ASTTOK_LITERAL)
                    {
                        str = nn_astparser_compilestring(prs, &length);
                        string = nn_string_takelen(prs->pvm, str, length);
                        /* gc fix */
                        nn_vm_stackpush(prs->pvm, nn_value_fromobject(string));
                        nn_table_set(sw->table, nn_value_fromobject(string), jump);
                        /* gc fix */
                        nn_vm_stackpop(prs->pvm);
                    }
                    else if(nn_astparser_checknumber(prs))
                    {
                        nn_table_set(sw->table, nn_astparser_compilenumber(prs), jump);
                    }
                    else
                    {
                        /* pop the switch */
                        nn_vm_stackpop(prs->pvm);
                        nn_astparser_raiseerror(prs, "only constants can be used in 'when' expressions");
                        return;
                    }
                } while(nn_astparser_match(prs, NEON_ASTTOK_COMMA));
                nn_astparser_consume(prs, NEON_ASTTOK_COLON, "expected ':' after 'case' constants");
                
            }
            else
            {
                nn_astparser_consume(prs, NEON_ASTTOK_COLON, "expected ':' after 'default'");
                swstate = 2;
                sw->defaultjump = nn_astparser_currentblob(prs)->count - startoffset;
            }
        }
        else
        {
            /* otherwise, it's a statement inside the current case */
            if(swstate == 0)
            {
                nn_astparser_raiseerror(prs, "cannot have statements before any case");
            }
            nn_astparser_parsestmt(prs);
        }
    }
    /* if we ended without a default case, patch its condition jump */
    if(swstate == 1)
    {
        caseends[casecount++] = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
    }
    /* patch all the case jumps to the end */
    for(i = 0; i < casecount; i++)
    {
        nn_astemit_patchjump(prs, caseends[i]);
    }
    sw->exitjump = nn_astparser_currentblob(prs)->count - startoffset;
    nn_astemit_patchswitch(prs, switchcode, nn_astparser_pushconst(prs, nn_value_fromobject(sw)));
    /* pop the switch */  
    nn_vm_stackpop(prs->pvm);
}

void nn_astparser_parseifstmt(NNAstParser* prs)
{
    int elsejump;
    int thenjump;
    nn_astparser_parseexpression(prs);
    thenjump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_parsestmt(prs);
    elsejump = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
    nn_astemit_patchjump(prs, thenjump);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    if(nn_astparser_match(prs, NEON_ASTTOK_KWELSE))
    {
        nn_astparser_parsestmt(prs);
    }
    nn_astemit_patchjump(prs, elsejump);
}

void nn_astparser_parseechostmt(NNAstParser* prs)
{
    nn_astparser_parseexpression(prs);
    nn_astemit_emitinstruc(prs, NEON_OP_ECHO);
    nn_astparser_consumestmtend(prs);
}

void nn_astparser_parsethrowstmt(NNAstParser* prs)
{
    nn_astparser_parseexpression(prs);
    nn_astemit_emitinstruc(prs, NEON_OP_EXTHROW);
    nn_astparser_discardlocals(prs, prs->currentfunccompiler->scopedepth - 1);
    nn_astparser_consumestmtend(prs);
}

void nn_astparser_parseassertstmt(NNAstParser* prs)
{
    nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' after 'assert'");
    nn_astparser_parseexpression(prs);
    if(nn_astparser_match(prs, NEON_ASTTOK_COMMA))
    {
        nn_astparser_ignorewhitespace(prs);
        nn_astparser_parseexpression(prs);
    }
    else
    {
        nn_astemit_emitinstruc(prs, NEON_OP_PUSHNULL);
    }
    nn_astemit_emitinstruc(prs, NEON_OP_ASSERT);
    nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after 'assert'");
    nn_astparser_consumestmtend(prs);
}

void nn_astparser_parsetrystmt(NNAstParser* prs)
{
    int address;
    int type;
    int finally;
    int trybegins;
    int exitjump;
    int continueexecutionaddress;
    bool catchexists;
    bool finalexists;
    if(prs->currentfunccompiler->handlercount == NEON_CFG_MAXEXCEPTHANDLERS)
    {
        nn_astparser_raiseerror(prs, "maximum exception handler in scope exceeded");
    }
    prs->currentfunccompiler->handlercount++;
    prs->istrying = true;
    nn_astparser_ignorewhitespace(prs);
    trybegins = nn_astemit_emittry(prs);
    /* compile the try body */
    nn_astparser_parsestmt(prs);
    nn_astemit_emitinstruc(prs, NEON_OP_EXPOPTRY);
    exitjump = nn_astemit_emitjump(prs, NEON_OP_JUMPNOW);
    prs->istrying = false;
    /*
    // we can safely use 0 because a program cannot start with a
    // catch or finally block
    */
    address = 0;
    type = -1;
    finally = 0;
    catchexists = false;
    finalexists= false;
    /* catch body must maintain its own scope */
    if(nn_astparser_match(prs, NEON_ASTTOK_KWCATCH))
    {
        catchexists = true;
        nn_astparser_scopebegin(prs);
        nn_astparser_consume(prs, NEON_ASTTOK_PARENOPEN, "expected '(' after 'catch'");
        nn_astparser_consume(prs, NEON_ASTTOK_IDENTNORMAL, "missing exception class name");
        type = nn_astparser_makeidentconst(prs, &prs->prevtoken);
        address = nn_astparser_currentblob(prs)->count;
        if(nn_astparser_match(prs, NEON_ASTTOK_IDENTNORMAL))
        {
            nn_astparser_createdvar(prs, prs->prevtoken);
        }
        else
        {
            nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        }
          nn_astparser_consume(prs, NEON_ASTTOK_PARENCLOSE, "expected ')' after 'catch'");
        nn_astemit_emitinstruc(prs, NEON_OP_EXPOPTRY);
        nn_astparser_ignorewhitespace(prs);
        nn_astparser_parsestmt(prs);
        nn_astparser_scopeend(prs);
    }
    else
    {
        type = nn_astparser_pushconst(prs, nn_value_fromobject(nn_string_copycstr(prs->pvm, "Exception")));
    }
    nn_astemit_patchjump(prs, exitjump);
    if(nn_astparser_match(prs, NEON_ASTTOK_KWFINALLY))
    {
        finalexists = true;
        /*
        // if we arrived here from either the try or handler block,
        // we don't want to continue propagating the exception
        */
        nn_astemit_emitinstruc(prs, NEON_OP_PUSHFALSE);
        finally = nn_astparser_currentblob(prs)->count;
        nn_astparser_ignorewhitespace(prs);
        nn_astparser_parsestmt(prs);
        continueexecutionaddress = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
        /* pop the bool off the stack */
        nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        nn_astemit_emitinstruc(prs, NEON_OP_EXPUBLISHTRY);
        nn_astemit_patchjump(prs, continueexecutionaddress);
        nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    }
    if(!finalexists && !catchexists)
    {
        nn_astparser_raiseerror(prs, "try block must contain at least one of catch or finally");
    }
    nn_astemit_patchtry(prs, trybegins, type, address, finally);
}

void nn_astparser_parsereturnstmt(NNAstParser* prs)
{
    prs->isreturning = true;
    /*
    if(prs->currentfunccompiler->type == NEON_FUNCTYPE_SCRIPT)
    {
        nn_astparser_raiseerror(prs, "cannot return from top-level code");
    }
    */
    if(nn_astparser_match(prs, NEON_ASTTOK_SEMICOLON) || nn_astparser_match(prs, NEON_ASTTOK_NEWLINE))
    {
        nn_astemit_emitreturn(prs);
    }
    else
    {
        if(prs->currentfunccompiler->type == NEON_FUNCTYPE_INITIALIZER)
        {
            nn_astparser_raiseerror(prs, "cannot return value from constructor");
        }
        if(prs->istrying)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_EXPOPTRY);
        }
        nn_astparser_parseexpression(prs);
        nn_astemit_emitinstruc(prs, NEON_OP_RETURN);
        nn_astparser_consumestmtend(prs);
    }
    prs->isreturning = false;
}

void nn_astparser_parsewhilestmt(NNAstParser* prs)
{
    int exitjump;
    int surroundingloopstart;
    int surroundingscopedepth;
    surroundingloopstart = prs->innermostloopstart;
    surroundingscopedepth = prs->innermostloopscopedepth;
    /*
    // we'll be jumping back to right before the
    // expression after the loop body
    */
    prs->innermostloopstart = nn_astparser_currentblob(prs)->count;
    prs->innermostloopscopedepth = prs->currentfunccompiler->scopedepth;
    nn_astparser_parseexpression(prs);
    exitjump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_parsestmt(prs);
    nn_astemit_emitloop(prs, prs->innermostloopstart);
    nn_astemit_patchjump(prs, exitjump);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_endloop(prs);
    prs->innermostloopstart = surroundingloopstart;
    prs->innermostloopscopedepth = surroundingscopedepth;
}

void nn_astparser_parsedo_whilestmt(NNAstParser* prs)
{
    int exitjump;
    int surroundingloopstart;
    int surroundingscopedepth;
    surroundingloopstart = prs->innermostloopstart;
    surroundingscopedepth = prs->innermostloopscopedepth;
    /*
    // we'll be jumping back to right before the
    // statements after the loop body
    */
    prs->innermostloopstart = nn_astparser_currentblob(prs)->count;
    prs->innermostloopscopedepth = prs->currentfunccompiler->scopedepth;
    nn_astparser_parsestmt(prs);
    nn_astparser_consume(prs, NEON_ASTTOK_KWWHILE, "expecting 'while' statement");
    nn_astparser_parseexpression(prs);
    exitjump = nn_astemit_emitjump(prs, NEON_OP_JUMPIFFALSE);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astemit_emitloop(prs, prs->innermostloopstart);
    nn_astemit_patchjump(prs, exitjump);
    nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
    nn_astparser_endloop(prs);
    prs->innermostloopstart = surroundingloopstart;
    prs->innermostloopscopedepth = surroundingscopedepth;
}

void nn_astparser_parsecontinuestmt(NNAstParser* prs)
{
    if(prs->innermostloopstart == -1)
    {
        nn_astparser_raiseerror(prs, "'continue' can only be used in a loop");
    }
    /*
    // discard local variables created in the loop
    //  discard_local(prs, prs->innermostloopscopedepth);
    */
    nn_astparser_discardlocals(prs, prs->innermostloopscopedepth + 1);
    /* go back to the top of the loop */
    nn_astemit_emitloop(prs, prs->innermostloopstart);
    nn_astparser_consumestmtend(prs);
}

void nn_astparser_parsebreakstmt(NNAstParser* prs)
{
    if(prs->innermostloopstart == -1)
    {
        nn_astparser_raiseerror(prs, "'break' can only be used in a loop");
    }
    /* discard local variables created in the loop */
    /*
    int i;
    for(i = prs->currentfunccompiler->localcount - 1; i >= 0 && prs->currentfunccompiler->locals[i].depth >= prs->currentfunccompiler->scopedepth; i--)
    {
        if (prs->currentfunccompiler->locals[i].iscaptured)
        {
            nn_astemit_emitinstruc(prs, NEON_OP_UPVALUECLOSE);
        }
        else
        {
            nn_astemit_emitinstruc(prs, NEON_OP_POPONE);
        }
    }
    */
    nn_astparser_discardlocals(prs, prs->innermostloopscopedepth + 1);
    nn_astemit_emitjump(prs, NEON_OP_BREAK_PL);
    nn_astparser_consumestmtend(prs);
}

void nn_astparser_synchronize(NNAstParser* prs)
{
    prs->panicmode = false;
    while(prs->currtoken.type != NEON_ASTTOK_EOF)
    {
        if(prs->currtoken.type == NEON_ASTTOK_NEWLINE || prs->currtoken.type == NEON_ASTTOK_SEMICOLON)
        {
            return;
        }
        switch(prs->currtoken.type)
        {
            case NEON_ASTTOK_KWCLASS:
            case NEON_ASTTOK_KWFUNCTION:
            case NEON_ASTTOK_KWVAR:
            case NEON_ASTTOK_KWFOREACH:
            case NEON_ASTTOK_KWIF:
            case NEON_ASTTOK_KWSWITCH:
            case NEON_ASTTOK_KWCASE:
            case NEON_ASTTOK_KWFOR:
            case NEON_ASTTOK_KWDO:
            case NEON_ASTTOK_KWWHILE:
            case NEON_ASTTOK_KWECHO:
            case NEON_ASTTOK_KWASSERT:
            case NEON_ASTTOK_KWTRY:
            case NEON_ASTTOK_KWCATCH:
            case NEON_ASTTOK_KWTHROW:
            case NEON_ASTTOK_KWRETURN:
            case NEON_ASTTOK_KWSTATIC:
            case NEON_ASTTOK_KWTHIS:
            case NEON_ASTTOK_KWSUPER:
            case NEON_ASTTOK_KWFINALLY:
            case NEON_ASTTOK_KWIN:
            case NEON_ASTTOK_KWIMPORT:
            case NEON_ASTTOK_KWAS:
                return;
            default:
                /* do nothing */
            ;
        }
        nn_astparser_advance(prs);
    }
}

/*
* $keeplast: whether to emit code that retains or discards the value of the last statement/expression.
* SHOULD NOT BE USED FOR ORDINARY SCRIPTS as it will almost definitely result in the stack containing invalid values.
*/
NNObjFuncScript* nn_astparser_compilesource(NNState* state, NNObjModule* module, const char* source, NNBlob* blob, bool fromimport, bool keeplast)
{
    NNAstFuncCompiler compiler;
    NNAstLexer* lexer;
    NNAstParser* parser;
    NNObjFuncScript* function;
    (void)blob;
    NEON_ASTDEBUG(state, "module=%p source=[...] blob=[...] fromimport=%d keeplast=%d", module, fromimport, keeplast);
    lexer = nn_astlex_init(state, source);
    parser = nn_astparser_make(state, lexer, module, keeplast);
    nn_astfunccompiler_init(parser, &compiler, NEON_FUNCTYPE_SCRIPT, true);
    compiler.fromimport = fromimport;
    nn_astparser_runparser(parser);
    function = nn_astparser_endcompiler(parser);
    if(parser->haderror)
    {
        function = NULL;
    }
    nn_astlex_destroy(state, lexer);
    nn_astparser_destroy(state, parser);
    return function;
}

void nn_gcmem_markcompilerroots(NNState* state)
{
    (void)state;
    /*
    NNAstFuncCompiler* compiler;
    compiler = state->compiler;
    while(compiler != NULL)
    {
        nn_gcmem_markobject(state, (NNObject*)compiler->targetfunc);
        compiler = compiler->enclosing;
    }
    */
}

NNRegModule* nn_natmodule_load_null(NNState* state)
{
    static NNRegFunc modfuncs[] =
    {
        /* {"somefunc",   true,  myfancymodulefunction},*/
        {NULL, false, NULL},
    };

    static NNRegField modfields[] =
    {
        /*{"somefield", true, the_function_that_gets_called},*/
        {NULL, false, NULL},
    };
    static NNRegModule module;
    (void)state;
    module.name = "null";
    module.fields = modfields;
    module.functions = modfuncs;
    module.classes = NULL;
    module.preloader= NULL;
    module.unloader = NULL;
    return &module;
}

void nn_modfn_os_preloader(NNState* state)
{
    (void)state;
}

NNValue nn_modfn_os_readdir(NNState* state, NNArguments* args)
{
    const char* dirn;
    FSDirReader rd;
    FSDirItem itm;
    NNObjString* os;
    NNObjString* aval;
    NNObjArray* res;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    os = nn_value_asstring(args->args[0]);
    dirn = os->sbuf->data;
    if(fslib_diropen(&rd, dirn))
    {
        res = nn_array_make(state);
        while(fslib_dirread(&rd, &itm))
        {
            aval = nn_string_copycstr(state, itm.name);
            nn_array_push(res, nn_value_fromobject(aval));
        }
        fslib_dirclose(&rd);
        return nn_value_fromobject(res);
    }
    else
    {
        nn_exceptions_throw(state, "cannot open directory '%s'", dirn);
    }
    return nn_value_makeempty();
}

NNRegModule* nn_natmodule_load_os(NNState* state)
{
    static NNRegFunc modfuncs[] =
    {
        {"readdir",   true,  nn_modfn_os_readdir},
        {NULL,     false, NULL},
    };
    static NNRegField modfields[] =
    {
        /*{"platform", true, get_os_platform},*/
        {NULL,       false, NULL},
    };
    static NNRegModule module;
    (void)state;
    module.name = "os";
    module.fields = modfields;
    module.functions = modfuncs;
    module.classes = NULL;
    module.preloader= &nn_modfn_os_preloader;
    module.unloader = NULL;
    return &module;
}

NNValue nn_modfn_astscan_scan(NNState* state, NNArguments* args)
{
    const char* cstr;
    NNObjString* insrc;
    NNAstLexer* scn;
    NNObjArray* arr;
    NNObjDict* itm;
    NNAstToken token;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    insrc = nn_value_asstring(args->args[0]);
    scn = nn_astlex_init(state, insrc->sbuf->data);
    arr = nn_array_make(state);
    while(!nn_astlex_isatend(scn))
    {
        itm = nn_object_makedict(state);
        token = nn_astlex_scantoken(scn);
        nn_dict_addentrycstr(itm, "line", nn_value_makenumber(token.line));
        cstr = nn_astutil_toktype2str(token.type);
        nn_dict_addentrycstr(itm, "type", nn_value_fromobject(nn_string_copycstr(state, cstr + 12)));
        nn_dict_addentrycstr(itm, "source", nn_value_fromobject(nn_string_copylen(state, token.start, token.length)));
        nn_array_push(arr, nn_value_fromobject(itm));
    }
    nn_astlex_destroy(state, scn);
    return nn_value_fromobject(arr);
}

NNRegModule* nn_natmodule_load_astscan(NNState* state)
{
    NNRegModule* ret;
    static NNRegFunc modfuncs[] =
    {
        {"scan",   true,  nn_modfn_astscan_scan},
        {NULL,     false, NULL},
    };
    static NNRegField modfields[] =
    {
        {NULL,       false, NULL},
    };
    static NNRegModule module;
    (void)state;
    module.name = "astscan";
    module.fields = modfields;
    module.functions = modfuncs;
    module.classes = NULL;
    module.preloader= NULL;
    module.unloader = NULL;
    ret = &module;
    return ret;
}

NNModInitFN g_builtinmodules[] =
{
    nn_natmodule_load_null,
    nn_natmodule_load_os,
    nn_natmodule_load_astscan,
    NULL,
};

bool nn_import_loadnativemodule(NNState* state, NNModInitFN init_fn, char* importname, const char* source, void* dlw)
{
    int j;
    int k;
    NNValue v;
    NNValue fieldname;
    NNValue funcname;
    NNValue funcrealvalue;
    NNRegFunc func;
    NNRegField field;
    NNRegModule* module;
    NNObjModule* themodule;
    NNRegClass klassreg;
    NNObjString* classname;
    NNObjFuncNative* native;
    NNObjClass* klass;
    NNHashTable* tabdest;
    module = init_fn(state);
    if(module != NULL)
    {
        themodule = (NNObjModule*)nn_gcmem_protect(state, (NNObject*)nn_module_make(state, (char*)module->name, source, false));
        themodule->preloader = (void*)module->preloader;
        themodule->unloader = (void*)module->unloader;
        if(module->fields != NULL)
        {
            for(j = 0; module->fields[j].name != NULL; j++)
            {
                field = module->fields[j];
                fieldname = nn_value_fromobject(nn_gcmem_protect(state, (NNObject*)nn_string_copycstr(state, field.name)));
                v = field.fieldvalfn(state);
                nn_vm_stackpush(state, v);
                nn_table_set(themodule->deftable, fieldname, v);
                nn_vm_stackpop(state);
            }
        }
        if(module->functions != NULL)
        {
            for(j = 0; module->functions[j].name != NULL; j++)
            {
                func = module->functions[j];
                funcname = nn_value_fromobject(nn_gcmem_protect(state, (NNObject*)nn_string_copycstr(state, func.name)));
                funcrealvalue = nn_value_fromobject(nn_gcmem_protect(state, (NNObject*)nn_object_makefuncnative(state, func.function, func.name, NULL)));
                nn_vm_stackpush(state, funcrealvalue);
                nn_table_set(themodule->deftable, funcname, funcrealvalue);
                nn_vm_stackpop(state);
            }
        }
        if(module->classes != NULL)
        {
            for(j = 0; module->classes[j].name != NULL; j++)
            {
                klassreg = module->classes[j];
                classname = (NNObjString*)nn_gcmem_protect(state, (NNObject*)nn_string_copycstr(state, klassreg.name));
                klass = (NNObjClass*)nn_gcmem_protect(state, (NNObject*)nn_object_makeclass(state, classname));
                if(klassreg.functions != NULL)
                {
                    for(k = 0; klassreg.functions[k].name != NULL; k++)
                    {
                        func = klassreg.functions[k];
                        funcname = nn_value_fromobject(nn_gcmem_protect(state, (NNObject*)nn_string_copycstr(state, func.name)));
                        native = (NNObjFuncNative*)nn_gcmem_protect(state, (NNObject*)nn_object_makefuncnative(state, func.function, func.name, NULL));
                        if(func.isstatic)
                        {
                            native->type = NEON_FUNCTYPE_STATIC;
                        }
                        else if(strlen(func.name) > 0 && func.name[0] == '_')
                        {
                            native->type = NEON_FUNCTYPE_PRIVATE;
                        }
                        nn_table_set(klass->methods, funcname, nn_value_fromobject(native));
                    }
                }
                if(klassreg.fields != NULL)
                {
                    for(k = 0; klassreg.fields[k].name != NULL; k++)
                    {
                        field = klassreg.fields[k];
                        fieldname = nn_value_fromobject(nn_gcmem_protect(state, (NNObject*)nn_string_copycstr(state, field.name)));
                        v = field.fieldvalfn(state);
                        nn_vm_stackpush(state, v);
                        tabdest = klass->instprops;
                        if(field.isstatic)
                        {
                            tabdest = klass->staticproperties;
                        }
                        nn_table_set(tabdest, fieldname, v);
                        nn_vm_stackpop(state);
                    }
                }
                nn_table_set(themodule->deftable, nn_value_fromobject(classname), nn_value_fromobject(klass));
            }
        }
        if(dlw != NULL)
        {
            themodule->handle = dlw;
        }
        nn_import_addnativemodule(state, themodule, themodule->name->sbuf->data);
        nn_gcmem_clearprotect(state);
        return true;
    }
    else
    {
        nn_state_warn(state, "Error loading module: %s\n", importname);
    }
    return false;
}

void nn_import_addnativemodule(NNState* state, NNObjModule* module, const char* as)
{
    NNValue name;
    if(as != NULL)
    {
        module->name = nn_string_copycstr(state, as);
    }
    name = nn_value_fromobject(nn_string_copyobjstr(state, module->name));
    nn_vm_stackpush(state, name);
    nn_vm_stackpush(state, nn_value_fromobject(module));
    nn_table_set(state->modules, name, nn_value_fromobject(module));
    nn_vm_stackpopn(state, 2);
}

void nn_import_loadbuiltinmodules(NNState* state)
{
    int i;
    for(i = 0; g_builtinmodules[i] != NULL; i++)
    {
        nn_import_loadnativemodule(state, g_builtinmodules[i], NULL, "<__native__>", NULL);
    }
}

void nn_import_closemodule(void* hnd)
{
    (void)hnd;
}

bool nn_util_fsfileexists(NNState* state, const char* filepath)
{
    (void)state;
    #if !defined(NEON_PLAT_ISWINDOWS)
        return access(filepath, F_OK) == 0;
    #else
        struct stat sb;
        if(stat(filepath, &sb) == -1)
        {
            return false;
        }
        return true;
    #endif
}

bool nn_util_fsfileisfile(NNState* state, const char* filepath)
{
    (void)state;
    (void)filepath;
    return false;
}

bool nn_util_fsfileisdirectory(NNState* state, const char* filepath)
{
    (void)state;
    (void)filepath;
    return false;
}

NNObjModule* nn_import_loadmodulescript(NNState* state, NNObjModule* intomodule, NNObjString* modulename)
{
    int argc;
    size_t fsz;
    char* source;
    char* physpath;
    NNBlob blob;
    NNValue retv;
    NNValue callable;
    NNProperty* field;
    NNObjArray* args;
    NNObjString* os;
    NNObjModule* module;
    NNObjFuncClosure* closure;
    NNObjFuncScript* function;
    (void)os;
    (void)argc;
    (void)intomodule;
    field = nn_table_getfieldbyostr(state->modules, modulename);
    if(field != NULL)
    {
        return nn_value_asmodule(field->value);
    }
    physpath = nn_import_resolvepath(state, modulename->sbuf->data, intomodule->physicalpath->sbuf->data, NULL, false);
    if(physpath == NULL)
    {
        nn_exceptions_throw(state, "module not found: '%s'\n", modulename->sbuf->data);
        return NULL;
    }
    fprintf(stderr, "loading module from '%s'\n", physpath);
    source = nn_util_readfile(state, physpath, &fsz);
    if(source == NULL)
    {
        nn_exceptions_throw(state, "could not read import file %s", physpath);
        return NULL;
    }
    nn_blob_init(state, &blob);
    module = nn_module_make(state, modulename->sbuf->data, physpath, true);
    nn_util_memfree(state, physpath);
    function = nn_astparser_compilesource(state, module, source, &blob, true, false);
    nn_util_memfree(state, source);
    closure = nn_object_makefuncclosure(state, function);
    callable = nn_value_fromobject(closure);
    args = nn_object_makearray(state);
    argc = nn_nestcall_prepare(state, callable, nn_value_makenull(), args);
    if(!nn_nestcall_callfunction(state, callable, nn_value_makenull(), args, &retv))
    {
        nn_blob_destroy(state, &blob);
        nn_exceptions_throw(state, "failed to call compiled import closure");
        return NULL;
    }
    nn_blob_destroy(state, &blob);
    return module;
}

char* nn_import_resolvepath(NNState* state, char* modulename, const char* currentfile, char* rootfile, bool isrelative)
{
    size_t i;
    size_t mlen;
    size_t splen;
    char* path1;
    char* path2;
    char* retme;
    const char* cstrpath;
    struct stat stroot;
    struct stat stmod;
    NNObjString* pitem;
    StringBuffer* pathbuf;
    (void)rootfile;
    (void)isrelative;
    (void)stroot;
    (void)stmod;
    pathbuf = NULL;
    mlen = strlen(modulename);
    splen = state->importpath->listcount;
    for(i=0; i<splen; i++)
    {
        pitem = nn_value_asstring(state->importpath->listitems[i]);
        if(pathbuf == NULL)
        {
            pathbuf = dyn_strbuf_makeempty(pitem->sbuf->length + mlen + 5);
        }
        else
        {
            dyn_strbuf_reset(pathbuf);
        }
        dyn_strbuf_appendstrn(pathbuf, pitem->sbuf->data, pitem->sbuf->length);
        if(dyn_strbuf_containschar(pathbuf, '@'))
        {
            dyn_strbuf_charreplace(pathbuf, '@', modulename, mlen);
        }
        else
        {
            dyn_strbuf_appendstr(pathbuf, "/");
            dyn_strbuf_appendstr(pathbuf, modulename);
            dyn_strbuf_appendstr(pathbuf, NEON_CFG_FILEEXT);
        }
        cstrpath = pathbuf->data; 
        fprintf(stderr, "import: trying '%s' ... ", cstrpath);
        if(nn_util_fsfileexists(state, cstrpath))
        {
            fprintf(stderr, "found!\n");
            /* stop a core library from importing itself */
            #if 0
            if(stat(currentfile, &stroot) == -1)
            {
                fprintf(stderr, "resolvepath: failed to stat current file '%s'\n", currentfile);
                return NULL;
            }
            if(stat(cstrpath, &stmod) == -1)
            {
                fprintf(stderr, "resolvepath: failed to stat module file '%s'\n", cstrpath);
                return NULL;
            }
            if(stroot.st_ino == stmod.st_ino)
            {
                fprintf(stderr, "resolvepath: refusing to import itself\n");
                return NULL;
            }
            #endif
            path1 = osfn_realpath(cstrpath, NULL);
            path2 = osfn_realpath(currentfile, NULL);
            if(path1 != NULL && path2 != NULL)
            {
                if(memcmp(path1, path2, (int)strlen(path2)) == 0)
                {
                    nn_util_memfree(state, path1);
                    nn_util_memfree(state, path2);
                    fprintf(stderr, "resolvepath: refusing to import itself\n");
                    return NULL;
                }
                nn_util_memfree(state, path2);
                dyn_strbuf_destroy(pathbuf);
                pathbuf = NULL;
                retme = nn_util_strdup(state, path1);
                nn_util_memfree(state, path1);
                return retme;
            }
        }
        else
        {
            fprintf(stderr, "does not exist\n");
        }
    }
    if(pathbuf != NULL)
    {
        dyn_strbuf_destroy(pathbuf);
    }
    return NULL;
}

char* nn_util_fsgetbasename(NNState* state, char* path)
{
    (void)state;
    return osfn_basename(path);
}


NNValue nn_objfndict_length(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makenumber(nn_value_asdict(args->thisval)->names->listcount);
}

NNValue nn_objfndict_add(NNState* state, NNArguments* args)
{
    NNValue tempvalue;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 2);
    dict = nn_value_asdict(args->thisval);
    if(nn_table_get(dict->htab, args->args[0], &tempvalue))
    {
        NEON_RETURNERROR("duplicate key %s at add()", nn_value_tostring(state, args->args[0])->sbuf->data);
    }
    nn_dict_addentry(dict, args->args[0], args->args[1]);
    return nn_value_makeempty();
}

NNValue nn_objfndict_set(NNState* state, NNArguments* args)
{
    NNValue value;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 2);
    dict = nn_value_asdict(args->thisval);
    if(!nn_table_get(dict->htab, args->args[0], &value))
    {
        nn_dict_addentry(dict, args->args[0], args->args[1]);
    }
    else
    {
        nn_dict_setentry(dict, args->args[0], args->args[1]);
    }
    return nn_value_makeempty();
}

NNValue nn_objfndict_clear(NNState* state, NNArguments* args)
{
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = nn_value_asdict(args->thisval);
    nn_vallist_destroy(dict->names);
    nn_table_destroy(dict->htab);
    return nn_value_makeempty();
}

NNValue nn_objfndict_clone(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjDict* dict;
    NNObjDict* newdict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = nn_value_asdict(args->thisval);
    newdict = (NNObjDict*)nn_gcmem_protect(state, (NNObject*)nn_object_makedict(state));
    nn_table_addall(dict->htab, newdict->htab);
    for(i = 0; i < dict->names->listcount; i++)
    {
        nn_vallist_push(newdict->names, dict->names->listitems[i]);
    }
    return nn_value_fromobject(newdict);
}

NNValue nn_objfndict_compact(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjDict* dict;
    NNObjDict* newdict;
    NNValue tmpvalue;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = nn_value_asdict(args->thisval);
    newdict = (NNObjDict*)nn_gcmem_protect(state, (NNObject*)nn_object_makedict(state));
    for(i = 0; i < dict->names->listcount; i++)
    {
        nn_table_get(dict->htab, dict->names->listitems[i], &tmpvalue);
        if(!nn_value_compare(state, tmpvalue, nn_value_makenull()))
        {
            nn_dict_addentry(newdict, dict->names->listitems[i], tmpvalue);
        }
    }
    return nn_value_fromobject(newdict);
}

NNValue nn_objfndict_contains(NNState* state, NNArguments* args)
{
    NNValue value;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    dict = nn_value_asdict(args->thisval);
    return nn_value_makebool(nn_table_get(dict->htab, args->args[0], &value));
}

NNValue nn_objfndict_extend(NNState* state, NNArguments* args)
{
    size_t i;
    NNValue tmp;
    NNObjDict* dict;
    NNObjDict* dictcpy;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isdict);
    dict = nn_value_asdict(args->thisval);
    dictcpy = nn_value_asdict(args->args[0]);
    for(i = 0; i < dictcpy->names->listcount; i++)
    {
        if(!nn_table_get(dict->htab, dictcpy->names->listitems[i], &tmp))
        {
            nn_vallist_push(dict->names, dictcpy->names->listitems[i]);
        }
    }
    nn_table_addall(dictcpy->htab, dict->htab);
    return nn_value_makeempty();
}

NNValue nn_objfndict_get(NNState* state, NNArguments* args)
{
    NNObjDict* dict;
    NNProperty* field;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    dict = nn_value_asdict(args->thisval);
    field = nn_dict_getentry(dict, args->args[0]);
    if(field == NULL)
    {
        if(args->count == 1)
        {
            return nn_value_makenull();
        }
        else
        {
            return args->args[1];
        }
    }
    return field->value;
}

NNValue nn_objfndict_keys(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjDict* dict;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = nn_value_asdict(args->thisval);
    list = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < dict->names->listcount; i++)
    {
        nn_array_push(list, dict->names->listitems[i]);
    }
    return nn_value_fromobject(list);
}

NNValue nn_objfndict_values(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjDict* dict;
    NNObjArray* list;
    NNProperty* field;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = nn_value_asdict(args->thisval);
    list = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < dict->names->listcount; i++)
    {
        field = nn_dict_getentry(dict, dict->names->listitems[i]);
        nn_array_push(list, field->value);
    }
    return nn_value_fromobject(list);
}

NNValue nn_objfndict_remove(NNState* state, NNArguments* args)
{
    size_t i;
    int index;
    NNValue value;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    dict = nn_value_asdict(args->thisval);
    if(nn_table_get(dict->htab, args->args[0], &value))
    {
        nn_table_delete(dict->htab, args->args[0]);
        index = -1;
        for(i = 0; i < dict->names->listcount; i++)
        {
            if(nn_value_compare(state, dict->names->listitems[i], args->args[0]))
            {
                index = i;
                break;
            }
        }
        for(i = index; i < dict->names->listcount; i++)
        {
            dict->names->listitems[i] = dict->names->listitems[i + 1];
        }
        dict->names->listcount--;
        return value;
    }
    return nn_value_makenull();
}

NNValue nn_objfndict_isempty(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makebool(nn_value_asdict(args->thisval)->names->listcount == 0);
}

NNValue nn_objfndict_findkey(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    return nn_table_findkey(nn_value_asdict(args->thisval)->htab, args->args[0]);
}

NNValue nn_objfndict_tolist(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjArray* list;
    NNObjDict* dict;
    NNObjArray* namelist;
    NNObjArray* valuelist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = nn_value_asdict(args->thisval);
    namelist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    valuelist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < dict->names->listcount; i++)
    {
        nn_array_push(namelist, dict->names->listitems[i]);
        NNValue value;
        if(nn_table_get(dict->htab, dict->names->listitems[i], &value))
        {
            nn_array_push(valuelist, value);
        }
        else
        {
            /* theoretically impossible */
            nn_array_push(valuelist, nn_value_makenull());
        }
    }
    list = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    nn_array_push(list, nn_value_fromobject(namelist));
    nn_array_push(list, nn_value_fromobject(valuelist));
    return nn_value_fromobject(list);
}

NNValue nn_objfndict_iter(NNState* state, NNArguments* args)
{
    NNValue result;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check,  args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    dict = nn_value_asdict(args->thisval);
    if(nn_table_get(dict->htab, args->args[0], &result))
    {
        return result;
    }
    return nn_value_makenull();
}

NNValue nn_objfndict_itern(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    dict = nn_value_asdict(args->thisval);
    if(nn_value_isnull(args->args[0]))
    {
        if(dict->names->listcount == 0)
        {
            return nn_value_makebool(false);
        }
        return dict->names->listitems[0];
    }
    for(i = 0; i < dict->names->listcount; i++)
    {
        if(nn_value_compare(state, args->args[0], dict->names->listitems[i]) && (i + 1) < dict->names->listcount)
        {
            return dict->names->listitems[i + 1];
        }
    }
    return nn_value_makenull();
}

NNValue nn_objfndict_each(NNState* state, NNArguments* args)
{
    size_t i;
    int arity;
    NNValue value;
    NNValue callable;
    NNValue unused;
    NNObjDict* dict;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    dict = nn_value_asdict(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < dict->names->listcount; i++)
    {
        if(arity > 0)
        {
            nn_table_get(dict->htab, dict->names->listitems[i], &value);
            nestargs->varray->listitems[0] = value;
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = dict->names->listitems[i];
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &unused);
    }
    /* pop the argument list */
    nn_vm_stackpop(state);
    return nn_value_makeempty();
}

NNValue nn_objfndict_filter(NNState* state, NNArguments* args)
{
    size_t i;
    int arity;
    NNValue value;
    NNValue callable;
    NNValue result;
    NNObjDict* dict;
    NNObjArray* nestargs;
    NNObjDict* resultdict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    dict = nn_value_asdict(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    resultdict = (NNObjDict*)nn_gcmem_protect(state, (NNObject*)nn_object_makedict(state));
    for(i = 0; i < dict->names->listcount; i++)
    {
        nn_table_get(dict->htab, dict->names->listitems[i], &value);
        if(arity > 0)
        {
            nestargs->varray->listitems[0] = value;
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = dict->names->listitems[i];
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &result);
        if(!nn_value_isfalse(result))
        {
            nn_dict_addentry(resultdict, dict->names->listitems[i], value);
        }
    }
    /* pop the call list */
    nn_vm_stackpop(state);
    return nn_value_fromobject(resultdict);
}

NNValue nn_objfndict_some(NNState* state, NNArguments* args)
{
    size_t i;
    int arity;
    NNValue result;
    NNValue value;
    NNValue callable;
    NNObjDict* dict;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    dict = nn_value_asdict(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < dict->names->listcount; i++)
    {
        if(arity > 0)
        {
            nn_table_get(dict->htab, dict->names->listitems[i], &value);
            nestargs->varray->listitems[0] = value;
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = dict->names->listitems[i];
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &result);
        if(!nn_value_isfalse(result))
        {
            /* pop the call list */
            nn_vm_stackpop(state);
            return nn_value_makebool(true);
        }
    }
    /* pop the call list */
    nn_vm_stackpop(state);
    return nn_value_makebool(false);
}


NNValue nn_objfndict_every(NNState* state, NNArguments* args)
{
    size_t i;
    int arity;
    NNValue value;
    NNValue callable;  
    NNValue result;
    NNObjDict* dict;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    dict = nn_value_asdict(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < dict->names->listcount; i++)
    {
        if(arity > 0)
        {
            nn_table_get(dict->htab, dict->names->listitems[i], &value);
            nestargs->varray->listitems[0] = value;
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = dict->names->listitems[i];
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &result);
        if(nn_value_isfalse(result))
        {
            /* pop the call list */
            nn_vm_stackpop(state);
            return nn_value_makebool(false);
        }
    }
    /* pop the call list */
    nn_vm_stackpop(state);
    return nn_value_makebool(true);
}

NNValue nn_objfndict_reduce(NNState* state, NNArguments* args)
{
    size_t i;
    int arity;
    int startindex;
    NNValue value;
    NNValue callable;
    NNValue accumulator;
    NNObjDict* dict;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    dict = nn_value_asdict(args->thisval);
    callable = args->args[0];
    startindex = 0;
    accumulator = nn_value_makenull();
    if(args->count == 2)
    {
        accumulator = args->args[1];
    }
    if(nn_value_isnull(accumulator) && dict->names->listcount > 0)
    {
        nn_table_get(dict->htab, dict->names->listitems[0], &accumulator);
        startindex = 1;
    }
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = startindex; i < dict->names->listcount; i++)
    {
        /* only call map for non-empty values in a list. */
        if(!nn_value_isnull(dict->names->listitems[i]) && !nn_value_isempty(dict->names->listitems[i]))
        {
            if(arity > 0)
            {
                nestargs->varray->listitems[0] = accumulator;
                if(arity > 1)
                {
                    nn_table_get(dict->htab, dict->names->listitems[i], &value);
                    nestargs->varray->listitems[1] = value;
                    if(arity > 2)
                    {
                        nestargs->varray->listitems[2] = dict->names->listitems[i];
                        if(arity > 4)
                        {
                            nestargs->varray->listitems[3] = args->thisval;
                        }
                    }
                }
            }
            nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &accumulator);
        }
    }
    /* pop the call list */
    nn_vm_stackpop(state);
    return accumulator;
}



#define FILE_ERROR(type, message) \
    NEON_RETURNERROR(#type " -> %s", message, file->path->sbuf->data);

#define RETURN_STATUS(status) \
    if((status) == 0) \
    { \
        return nn_value_makebool(true); \
    } \
    else \
    { \
        FILE_ERROR(File, strerror(errno)); \
    }

#define DENY_STD() \
    if(file->isstd) \
    NEON_RETURNERROR("method not supported for std files");

int nn_fileobject_close(NNObjFile* file)
{
    int result;
    if(file->handle != NULL && !file->isstd)
    {
        fflush(file->handle);
        result = fclose(file->handle);
        file->handle = NULL;
        file->isopen = false;
        file->number = -1;
        file->istty = false;
        return result;
    }
    return -1;
}

bool nn_fileobject_open(NNObjFile* file)
{
    if(file->handle != NULL)
    {
        return true;
    }
    if(file->handle == NULL && !file->isstd)
    {
        file->handle = fopen(file->path->sbuf->data, file->mode->sbuf->data);
        if(file->handle != NULL)
        {
            file->isopen = true;
            file->number = fileno(file->handle);
            file->istty = osfn_isatty(file->number);
            return true;
        }
        else
        {
            file->number = -1;
            file->istty = false;
        }
        return false;
    }
    return false;
}

NNValue nn_objfnfile_constructor(NNState* state, NNArguments* args)
{
    FILE* hnd;
    const char* path;
    const char* mode;
    NNObjString* opath;
    NNObjFile* file;
    (void)hnd;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    opath = nn_value_asstring(args->args[0]);
    if(opath->sbuf->length == 0)
    {
        NEON_RETURNERROR("file path cannot be empty");
    }
    mode = "r";
    if(args->count == 2)
    {
        NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isstring);
        mode = nn_value_asstring(args->args[1])->sbuf->data;
    }
    path = opath->sbuf->data;
    file = (NNObjFile*)nn_gcmem_protect(state, (NNObject*)nn_object_makefile(state, NULL, false, path, mode));
    nn_fileobject_open(file);
    return nn_value_fromobject(file);
}

NNValue nn_objfnfile_exists(NNState* state, NNArguments* args)
{
    NNObjString* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    file = nn_value_asstring(args->args[0]);
    return nn_value_makebool(nn_util_fsfileexists(state, file->sbuf->data));
}

NNValue nn_objfnfile_isfile(NNState* state, NNArguments* args)
{
    NNObjString* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    file = nn_value_asstring(args->args[0]);
    return nn_value_makebool(nn_util_fsfileisfile(state, file->sbuf->data));
}

NNValue nn_objfnfile_isdirectory(NNState* state, NNArguments* args)
{
    NNObjString* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    file = nn_value_asstring(args->args[0]);
    return nn_value_makebool(nn_util_fsfileisdirectory(state, file->sbuf->data));
}

NNValue nn_objfnfile_close(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    nn_fileobject_close(nn_value_asfile(args->thisval));
    return nn_value_makeempty();
}

NNValue nn_objfnfile_open(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    nn_fileobject_open(nn_value_asfile(args->thisval));
    return nn_value_makeempty();
}

NNValue nn_objfnfile_isopen(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    (void)state;
    file = nn_value_asfile(args->thisval);
    return nn_value_makebool(file->isstd || file->isopen);
}

NNValue nn_objfnfile_isclosed(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    (void)state;
    file = nn_value_asfile(args->thisval);
    return nn_value_makebool(!file->isstd && !file->isopen);
}

bool nn_file_read(NNState* state, NNObjFile* file, size_t readhowmuch, NNIOResult* dest)
{
    size_t filesizereal;
    struct stat stats;
    filesizereal = -1;
    dest->success = false;
    dest->length = 0;
    dest->data = NULL;
    if(!file->isstd)
    {
        if(!nn_util_fsfileexists(state, file->path->sbuf->data))
        {
            return false;
        }
        /* file is in write only mode */
        /*
        else if(strstr(file->mode->sbuf->data, "w") != NULL && strstr(file->mode->sbuf->data, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot read file in write mode");
        }
        */
        if(!file->isopen)
        {
            /* open the file if it isn't open */
            nn_fileobject_open(file);
        }
        else if(file->handle == NULL)
        {
            return false;
        }
        if(osfn_lstat(file->path->sbuf->data, &stats) == 0)
        {
            filesizereal = (size_t)stats.st_size;
        }
        else
        {
            /* fallback */
            fseek(file->handle, 0L, SEEK_END);
            filesizereal = ftell(file->handle);
            rewind(file->handle);
        }
        if(readhowmuch == (size_t)-1 || readhowmuch > filesizereal)
        {
            readhowmuch = filesizereal;
        }
    }
    else
    {
        /*
        // for non-file objects such as stdin
        // minimum read bytes should be 1
        */
        if(readhowmuch == (size_t)-1)
        {
            readhowmuch = 1;
        }
    }
    /* +1 for terminator '\0' */
    dest->data = (char*)nn_gcmem_allocate(state, sizeof(char), readhowmuch + 1);
    if(dest->data == NULL && readhowmuch != 0)
    {
        return false;
    }
    dest->length = fread(dest->data, sizeof(char), readhowmuch, file->handle);
    if(dest->length == 0 && readhowmuch != 0 && readhowmuch == filesizereal)
    {
        return false;
    }
    /* we made use of +1 so we can terminate the string. */
    if(dest->data != NULL)
    {
        dest->data[dest->length] = '\0';
    }
    return true;
}

NNValue nn_objfnfile_read(NNState* state, NNArguments* args)
{
    size_t readhowmuch;
    NNIOResult res;
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    readhowmuch = -1;
    if(args->count == 1)
    {
        NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
        readhowmuch = (size_t)nn_value_asnumber(args->args[0]);
    }
    file = nn_value_asfile(args->thisval);
    if(!nn_file_read(state, file, readhowmuch, &res))
    {
        FILE_ERROR(NotFound, strerror(errno));
    }
    return nn_value_fromobject(nn_string_takelen(state, res.data, res.length));
}

NNValue nn_objfnfile_get(NNState* state, NNArguments* args)
{
    int ch;
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    ch = fgetc(file->handle);
    if(ch == EOF)
    {
        return nn_value_makenull();
    }
    return nn_value_makenumber(ch);
}

NNValue nn_objfnfile_gets(NNState* state, NNArguments* args)
{
    long end;
    long length;
    long currentpos;
    size_t bytesread;
    char* buffer;
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    length = -1;
    if(args->count == 1)
    {
        NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
        length = (size_t)nn_value_asnumber(args->args[0]);
    }
    file = nn_value_asfile(args->thisval);
    if(!file->isstd)
    {
        if(!nn_util_fsfileexists(state, file->path->sbuf->data))
        {
            FILE_ERROR(NotFound, "no such file or directory");
        }
        else if(strstr(file->mode->sbuf->data, "w") != NULL && strstr(file->mode->sbuf->data, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot read file in write mode");
        }
        if(!file->isopen)
        {
            FILE_ERROR(Read, "file not open");
        }
        else if(file->handle == NULL)
        {
            FILE_ERROR(Read, "could not read file");
        }
        if(length == -1)
        {
            currentpos = ftell(file->handle);
            fseek(file->handle, 0L, SEEK_END);
            end = ftell(file->handle);
            fseek(file->handle, currentpos, SEEK_SET);
            length = end - currentpos;
        }
    }
    else
    {
        if(fileno(stdout) == file->number || fileno(stderr) == file->number)
        {
            FILE_ERROR(Unsupported, "cannot read from output file");
        }
        /*
        // for non-file objects such as stdin
        // minimum read bytes should be 1
        */
        if(length == -1)
        {
            length = 1;
        }
    }
    buffer = (char*)nn_gcmem_allocate(state, sizeof(char), length + 1);
    if(buffer == NULL && length != 0)
    {
        FILE_ERROR(Buffer, "not enough memory to read file");
    }
    bytesread = fread(buffer, sizeof(char), length, file->handle);
    if(bytesread == 0 && length != 0)
    {
        FILE_ERROR(Read, "could not read file contents");
    }
    if(buffer != NULL)
    {
        buffer[bytesread] = '\0';
    }
    return nn_value_fromobject(nn_string_takelen(state, buffer, bytesread));
}

NNValue nn_objfnfile_write(NNState* state, NNArguments* args)
{
    size_t count;
    int length;
    unsigned char* data;
    NNObjFile* file;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    file = nn_value_asfile(args->thisval);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->args[0]);
    data = (unsigned char*)string->sbuf->data;
    length = string->sbuf->length;
    if(!file->isstd)
    {
        if(strstr(file->mode->sbuf->data, "r") != NULL && strstr(file->mode->sbuf->data, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot write into non-writable file");
        }
        else if(length == 0)
        {
            FILE_ERROR(Write, "cannot write empty buffer to file");
        }
        else if(file->handle == NULL || !file->isopen)
        {
            nn_fileobject_open(file);
        }
        else if(file->handle == NULL)
        {
            FILE_ERROR(Write, "could not write to file");
        }
    }
    else
    {
        if(fileno(stdin) == file->number)
        {
            FILE_ERROR(Unsupported, "cannot write to input file");
        }
    }
    count = fwrite(data, sizeof(unsigned char), length, file->handle);
    fflush(file->handle);
    if(count > (size_t)0)
    {
        return nn_value_makebool(true);
    }
    return nn_value_makebool(false);
}

NNValue nn_objfnfile_puts(NNState* state, NNArguments* args)
{
    size_t count;
    int length;
    unsigned char* data;
    NNObjFile* file;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    file = nn_value_asfile(args->thisval);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->args[0]);
    data = (unsigned char*)string->sbuf->data;
    length = string->sbuf->length;
    if(!file->isstd)
    {
        if(strstr(file->mode->sbuf->data, "r") != NULL && strstr(file->mode->sbuf->data, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot write into non-writable file");
        }
        else if(length == 0)
        {
            FILE_ERROR(Write, "cannot write empty buffer to file");
        }
        else if(!file->isopen)
        {
            FILE_ERROR(Write, "file not open");
        }
        else if(file->handle == NULL)
        {
            FILE_ERROR(Write, "could not write to file");
        }
    }
    else
    {
        if(fileno(stdin) == file->number)
        {
            FILE_ERROR(Unsupported, "cannot write to input file");
        }
    }
    count = fwrite(data, sizeof(unsigned char), length, file->handle);
    if(count > (size_t)0 || length == 0)
    {
        return nn_value_makebool(true);
    }
    return nn_value_makebool(false);
}

NNValue nn_objfnfile_printf(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    NNFormatInfo nfi;
    NNPrinter pr;
    NNObjString* ofmt;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    file = nn_value_asfile(args->thisval);
    NEON_ARGS_CHECKMINARG(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    ofmt = nn_value_asstring(args->args[0]);
    nn_printer_makestackio(state, &pr, file->handle, false);
    nn_strformat_init(state, &nfi, &pr, nn_string_getcstr(ofmt), nn_string_getlength(ofmt));
    if(!nn_strformat_format(&nfi, args->count, 1, args->args))
    {
    }
    return nn_value_makenull();
}

NNValue nn_objfnfile_number(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makenumber(nn_value_asfile(args->thisval)->number);
}

NNValue nn_objfnfile_istty(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    return nn_value_makebool(file->istty);
}

NNValue nn_objfnfile_flush(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    if(!file->isopen)
    {
        FILE_ERROR(Unsupported, "I/O operation on closed file");
    }
    #if defined(NEON_PLAT_ISLINUX)
    if(fileno(stdin) == file->number)
    {
        while((getchar()) != '\n')
        {
        }
    }
    else
    {
        fflush(file->handle);
    }
    #else
    fflush(file->handle);
    #endif
    return nn_value_makeempty();
}

NNValue nn_objfnfile_stats(NNState* state, NNArguments* args)
{
    struct stat stats;
    NNObjFile* file;
    NNObjDict* dict;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    dict = (NNObjDict*)nn_gcmem_protect(state, (NNObject*)nn_object_makedict(state));
    if(!file->isstd)
    {
        if(nn_util_fsfileexists(state, file->path->sbuf->data))
        {
            if(osfn_lstat(file->path->sbuf->data, &stats) == 0)
            {
                #if !defined(NEON_PLAT_ISWINDOWS)
                nn_dict_addentrycstr(dict, "isreadable", nn_value_makebool(((stats.st_mode & S_IRUSR) != 0)));
                nn_dict_addentrycstr(dict, "iswritable", nn_value_makebool(((stats.st_mode & S_IWUSR) != 0)));
                nn_dict_addentrycstr(dict, "isexecutable", nn_value_makebool(((stats.st_mode & S_IXUSR) != 0)));
                nn_dict_addentrycstr(dict, "issymbolic", nn_value_makebool((S_ISLNK(stats.st_mode) != 0)));
                #else
                nn_dict_addentrycstr(dict, "isreadable", nn_value_makebool(((stats.st_mode & S_IREAD) != 0)));
                nn_dict_addentrycstr(dict, "iswritable", nn_value_makebool(((stats.st_mode & S_IWRITE) != 0)));
                nn_dict_addentrycstr(dict, "isexecutable", nn_value_makebool(((stats.st_mode & S_IEXEC) != 0)));
                nn_dict_addentrycstr(dict, "issymbolic", nn_value_makebool(false));
                #endif
                nn_dict_addentrycstr(dict, "size", nn_value_makenumber(stats.st_size));
                nn_dict_addentrycstr(dict, "mode", nn_value_makenumber(stats.st_mode));
                nn_dict_addentrycstr(dict, "dev", nn_value_makenumber(stats.st_dev));
                nn_dict_addentrycstr(dict, "ino", nn_value_makenumber(stats.st_ino));
                nn_dict_addentrycstr(dict, "nlink", nn_value_makenumber(stats.st_nlink));
                nn_dict_addentrycstr(dict, "uid", nn_value_makenumber(stats.st_uid));
                nn_dict_addentrycstr(dict, "gid", nn_value_makenumber(stats.st_gid));
                nn_dict_addentrycstr(dict, "mtime", nn_value_makenumber(stats.st_mtime));
                nn_dict_addentrycstr(dict, "atime", nn_value_makenumber(stats.st_atime));
                nn_dict_addentrycstr(dict, "ctime", nn_value_makenumber(stats.st_ctime));
                nn_dict_addentrycstr(dict, "blocks", nn_value_makenumber(0));
                nn_dict_addentrycstr(dict, "blksize", nn_value_makenumber(0));
            }
        }
        else
        {
            NEON_RETURNERROR("cannot get stats for non-existing file");
        }
    }
    else
    {
        if(fileno(stdin) == file->number)
        {
            nn_dict_addentrycstr(dict, "isreadable", nn_value_makebool(true));
            nn_dict_addentrycstr(dict, "iswritable", nn_value_makebool(false));
        }
        else
        {
            nn_dict_addentrycstr(dict, "isreadable", nn_value_makebool(false));
            nn_dict_addentrycstr(dict, "iswritable", nn_value_makebool(true));
        }
        nn_dict_addentrycstr(dict, "isexecutable", nn_value_makebool(false));
        nn_dict_addentrycstr(dict, "size", nn_value_makenumber(1));
    }
    return nn_value_fromobject(dict);
}

NNValue nn_objfnfile_path(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    DENY_STD();
    return nn_value_fromobject(file->path);
}

NNValue nn_objfnfile_mode(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    return nn_value_fromobject(file->mode);
}

NNValue nn_objfnfile_name(NNState* state, NNArguments* args)
{
    char* name;
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    if(!file->isstd)
    {
        name = nn_util_fsgetbasename(state, file->path->sbuf->data);
        return nn_value_fromobject(nn_string_copycstr(state, name));
    }
    else if(file->istty)
    {
        /*name = ttyname(file->number);*/
        name = nn_util_strdup(state, "<tty>");
        if(name)
        {
            return nn_value_fromobject(nn_string_copycstr(state, name));
        }
    }
    return nn_value_makenull();
}

NNValue nn_objfnfile_seek(NNState* state, NNArguments* args)
{
    long position;
    int seektype;
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
    file = nn_value_asfile(args->thisval);
    DENY_STD();
    position = (long)nn_value_asnumber(args->args[0]);
    seektype = nn_value_asnumber(args->args[1]);
    RETURN_STATUS(fseek(file->handle, position, seektype));
}

NNValue nn_objfnfile_tell(NNState* state, NNArguments* args)
{
    NNObjFile* file;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    file = nn_value_asfile(args->thisval);
    DENY_STD();
    return nn_value_makenumber(ftell(file->handle));
}

#undef FILE_ERROR
#undef RETURN_STATUS
#undef DENY_STD

NNObjArray* nn_array_makefilled(NNState* state, size_t cnt, NNValue filler)
{
    size_t i;
    NNObjArray* list;
    list = (NNObjArray*)nn_object_allocobject(state, sizeof(NNObjArray), NEON_OBJTYPE_ARRAY);
    list->varray = nn_vallist_make(state);
    if(cnt > 0)
    {
        for(i=0; i<cnt; i++)
        {
            nn_vallist_push(list->varray, filler);
        }
    }
    return list;
}

NNObjArray* nn_array_make(NNState* state)
{
    return nn_array_makefilled(state, 0, nn_value_makeempty());
}

void nn_array_push(NNObjArray* list, NNValue value)
{
    NNState* state;
    (void)state;
    state = ((NNObject*)list)->pvm;
    /*nn_vm_stackpush(state, value);*/
    nn_vallist_push(list->varray, value);
    /*nn_vm_stackpop(state); */
}

bool nn_array_get(NNObjArray* list, size_t idx, NNValue* vdest)
{
    size_t vc;
    vc = nn_vallist_count(list->varray);
    if((vc > 0) && (idx < vc))
    {
        *vdest = nn_vallist_get(list->varray, idx);
        return true;
    }
    return false;
}

NNObjArray* nn_array_copy(NNObjArray* list, long start, long length)
{
    size_t i;
    NNState* state;
    NNObjArray* newlist;
    state = ((NNObject*)list)->pvm;
    newlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    if(start == -1)
    {
        start = 0;
    }
    if(length == -1)
    {
        length = list->varray->listcount - start;
    }
    for(i = start; i < (size_t)(start + length); i++)
    {
        nn_array_push(newlist, list->varray->listitems[i]);
    }
    return newlist;
}

NNValue nn_objfnarray_length(NNState* state, NNArguments* args)
{
    NNObjArray* selfarr;
    (void)state;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    selfarr = nn_value_asarray(args->thisval);
    return nn_value_makenumber(selfarr->varray->listcount);
}

NNValue nn_objfnarray_append(NNState* state, NNArguments* args)
{
    size_t i;
    (void)state;
    for(i = 0; i < args->count; i++)
    {
        nn_array_push(nn_value_asarray(args->thisval), args->args[i]);
    }
    return nn_value_makeempty();
}

NNValue nn_objfnarray_clear(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    nn_vallist_destroy(nn_value_asarray(args->thisval)->varray);
    return nn_value_makeempty();
}

NNValue nn_objfnarray_clone(NNState* state, NNArguments* args)
{
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    return nn_value_fromobject(nn_array_copy(list, 0, list->varray->listcount));
}

NNValue nn_objfnarray_count(NNState* state, NNArguments* args)
{
    size_t i;
    int count;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    list = nn_value_asarray(args->thisval);
    count = 0;
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(nn_value_compare(state, list->varray->listitems[i], args->args[0]))
        {
            count++;
        }
    }
    return nn_value_makenumber(count);
}

NNValue nn_objfnarray_extend(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjArray* list;
    NNObjArray* list2;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isarray);
    list = nn_value_asarray(args->thisval);
    list2 = nn_value_asarray(args->args[0]);
    for(i = 0; i < list2->varray->listcount; i++)
    {
        nn_array_push(list, list2->varray->listitems[i]);
    }
    return nn_value_makeempty();
}

NNValue nn_objfnarray_indexof(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    list = nn_value_asarray(args->thisval);
    i = 0;
    if(args->count == 2)
    {
        NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
        i = nn_value_asnumber(args->args[1]);
    }
    for(; i < list->varray->listcount; i++)
    {
        if(nn_value_compare(state, list->varray->listitems[i], args->args[0]))
        {
            return nn_value_makenumber(i);
        }
    }
    return nn_value_makenumber(-1);
}

NNValue nn_objfnarray_insert(NNState* state, NNArguments* args)
{
    int index;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 2);
    NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
    list = nn_value_asarray(args->thisval);
    index = (int)nn_value_asnumber(args->args[1]);
    nn_vallist_insert(list->varray, args->args[0], index);
    return nn_value_makeempty();
}


NNValue nn_objfnarray_pop(NNState* state, NNArguments* args)
{
    NNValue value;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    if(list->varray->listcount > 0)
    {
        value = list->varray->listitems[list->varray->listcount - 1];
        list->varray->listcount--;
        return value;
    }
    return nn_value_makenull();
}

NNValue nn_objfnarray_shift(NNState* state, NNArguments* args)
{
    size_t i;
    size_t j;
    size_t count;
    NNObjArray* list;
    NNObjArray* newlist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    count = 1;
    if(args->count == 1)
    {
        NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
        count = nn_value_asnumber(args->args[0]);
    }
    list = nn_value_asarray(args->thisval);
    if(count >= list->varray->listcount || list->varray->listcount == 1)
    {
        list->varray->listcount = 0;
        return nn_value_makenull();
    }
    else if(count > 0)
    {
        newlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
        for(i = 0; i < count; i++)
        {
            nn_array_push(newlist, list->varray->listitems[0]);
            for(j = 0; j < list->varray->listcount; j++)
            {
                list->varray->listitems[j] = list->varray->listitems[j + 1];
            }
            list->varray->listcount -= 1;
        }
        if(count == 1)
        {
            return newlist->varray->listitems[0];
        }
        else
        {
            return nn_value_fromobject(newlist);
        }
    }
    return nn_value_makenull();
}

NNValue nn_objfnarray_removeat(NNState* state, NNArguments* args)
{
    size_t i;
    size_t index;
    NNValue value;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    list = nn_value_asarray(args->thisval);
    index = nn_value_asnumber(args->args[0]);
    if(((int)index < 0) || index >= list->varray->listcount)
    {
        NEON_RETURNERROR("list index %d out of range at remove_at()", index);
    }
    value = list->varray->listitems[index];
    for(i = index; i < list->varray->listcount - 1; i++)
    {
        list->varray->listitems[i] = list->varray->listitems[i + 1];
    }
    list->varray->listcount--;
    return value;
}

NNValue nn_objfnarray_remove(NNState* state, NNArguments* args)
{
    size_t i;
    size_t index;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    list = nn_value_asarray(args->thisval);
    index = -1;
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(nn_value_compare(state, list->varray->listitems[i], args->args[0]))
        {
            index = i;
            break;
        }
    }
    if((int)index != -1)
    {
        for(i = index; i < list->varray->listcount; i++)
        {
            list->varray->listitems[i] = list->varray->listitems[i + 1];
        }
        list->varray->listcount--;
    }
    return nn_value_makeempty();
}

NNValue nn_objfnarray_reverse(NNState* state, NNArguments* args)
{
    int fromtop;
    NNObjArray* list;
    NNObjArray* nlist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    nlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    /* in-place reversal:*/
    /*
    int start = 0;
    int end = list->varray->listcount - 1;
    while (start < end)
    {
        NNValue temp = list->varray->listitems[start];
        list->varray->listitems[start] = list->varray->listitems[end];
        list->varray->listitems[end] = temp;
        start++;
        end--;
    }
    */
    for(fromtop = list->varray->listcount - 1; fromtop >= 0; fromtop--)
    {
        nn_array_push(nlist, list->varray->listitems[fromtop]);
    }
    return nn_value_fromobject(nlist);
}

NNValue nn_objfnarray_sort(NNState* state, NNArguments* args)
{
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    nn_value_sortvalues(state, list->varray->listitems, list->varray->listcount);
    return nn_value_makeempty();
}

NNValue nn_objfnarray_contains(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    list = nn_value_asarray(args->thisval);
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(nn_value_compare(state, args->args[0], list->varray->listitems[i]))
        {
            return nn_value_makebool(true);
        }
    }
    return nn_value_makebool(false);
}

NNValue nn_objfnarray_delete(NNState* state, NNArguments* args)
{
    size_t i;
    size_t idxupper;
    size_t idxlower;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    idxlower = nn_value_asnumber(args->args[0]);
    idxupper = idxlower;
    if(args->count == 2)
    {
        NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
        idxupper = nn_value_asnumber(args->args[1]);
    }
    list = nn_value_asarray(args->thisval);
    if(((int)idxlower < 0) || idxlower >= list->varray->listcount)
    {
        NEON_RETURNERROR("list index %d out of range at delete()", idxlower);
    }
    else if(idxupper < idxlower || idxupper >= list->varray->listcount)
    {
        NEON_RETURNERROR("invalid upper limit %d at delete()", idxupper);
    }
    for(i = 0; i < list->varray->listcount - idxupper; i++)
    {
        list->varray->listitems[idxlower + i] = list->varray->listitems[i + idxupper + 1];
    }
    list->varray->listcount -= idxupper - idxlower + 1;
    return nn_value_makenumber((double)idxupper - (double)idxlower + 1);
}

NNValue nn_objfnarray_first(NNState* state, NNArguments* args)
{
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    if(list->varray->listcount > 0)
    {
        return list->varray->listitems[0];
    }
    return nn_value_makenull();
}

NNValue nn_objfnarray_last(NNState* state, NNArguments* args)
{
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    if(list->varray->listcount > 0)
    {
        return list->varray->listitems[list->varray->listcount - 1];
    }
    return nn_value_makenull();
}

NNValue nn_objfnarray_isempty(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makebool(nn_value_asarray(args->thisval)->varray->listcount == 0);
}


NNValue nn_objfnarray_take(NNState* state, NNArguments* args)
{
    size_t count;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    list = nn_value_asarray(args->thisval);
    count = nn_value_asnumber(args->args[0]);
    if((int)count < 0)
    {
        count = list->varray->listcount + count;
    }
    if(list->varray->listcount < count)
    {
        return nn_value_fromobject(nn_array_copy(list, 0, list->varray->listcount));
    }
    return nn_value_fromobject(nn_array_copy(list, 0, count));
}

NNValue nn_objfnarray_get(NNState* state, NNArguments* args)
{
    size_t index;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    list = nn_value_asarray(args->thisval);
    index = nn_value_asnumber(args->args[0]);
    if((int)index < 0 || index >= list->varray->listcount)
    {
        return nn_value_makenull();
    }
    return list->varray->listitems[index];
}

NNValue nn_objfnarray_compact(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjArray* list;
    NNObjArray* newlist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    newlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(!nn_value_compare(state, list->varray->listitems[i], nn_value_makenull()))
        {
            nn_array_push(newlist, list->varray->listitems[i]);
        }
    }
    return nn_value_fromobject(newlist);
}


NNValue nn_objfnarray_unique(NNState* state, NNArguments* args)
{
    size_t i;
    size_t j;
    bool found;
    NNObjArray* list;
    NNObjArray* newlist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    list = nn_value_asarray(args->thisval);
    newlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < list->varray->listcount; i++)
    {
        found = false;
        for(j = 0; j < newlist->varray->listcount; j++)
        {
            if(nn_value_compare(state, newlist->varray->listitems[j], list->varray->listitems[i]))
            {
                found = true;
                continue;
            }
        }
        if(!found)
        {
            nn_array_push(newlist, list->varray->listitems[i]);
        }
    }
    return nn_value_fromobject(newlist);
}

NNValue nn_objfnarray_zip(NNState* state, NNArguments* args)
{
    size_t i;
    size_t j;
    NNObjArray* list;
    NNObjArray* newlist;
    NNObjArray* alist;
    NNObjArray** arglist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    list = nn_value_asarray(args->thisval);
    newlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    arglist = (NNObjArray**)nn_gcmem_allocate(state, sizeof(NNObjArray*), args->count);
    for(i = 0; i < args->count; i++)
    {
        NEON_ARGS_CHECKTYPE(&check, i, nn_value_isarray);
        arglist[i] = nn_value_asarray(args->args[i]);
    }
    for(i = 0; i < list->varray->listcount; i++)
    {
        alist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
        /* item of main list*/
        nn_array_push(alist, list->varray->listitems[i]);
        for(j = 0; j < args->count; j++)
        {
            if(i < arglist[j]->varray->listcount)
            {
                nn_array_push(alist, arglist[j]->varray->listitems[i]);
            }
            else
            {
                nn_array_push(alist, nn_value_makenull());
            }
        }
        nn_array_push(newlist, nn_value_fromobject(alist));
    }
    return nn_value_fromobject(newlist);
}


NNValue nn_objfnarray_zipfrom(NNState* state, NNArguments* args)
{
    size_t i;
    size_t j;
    NNObjArray* list;
    NNObjArray* newlist;
    NNObjArray* alist;
    NNObjArray* arglist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isarray);
    list = nn_value_asarray(args->thisval);
    newlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    arglist = nn_value_asarray(args->args[0]);
    for(i = 0; i < arglist->varray->listcount; i++)
    {
        if(!nn_value_isarray(arglist->varray->listitems[i]))
        {
            NEON_RETURNERROR("invalid list in zip entries");
        }
    }
    for(i = 0; i < list->varray->listcount; i++)
    {
        alist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
        nn_array_push(alist, list->varray->listitems[i]);
        for(j = 0; j < arglist->varray->listcount; j++)
        {
            if(i < nn_value_asarray(arglist->varray->listitems[j])->varray->listcount)
            {
                nn_array_push(alist, nn_value_asarray(arglist->varray->listitems[j])->varray->listitems[i]);
            }
            else
            {
                nn_array_push(alist, nn_value_makenull());
            }
        }
        nn_array_push(newlist, nn_value_fromobject(alist));
    }
    return nn_value_fromobject(newlist);
}

NNValue nn_objfnarray_todict(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjDict* dict;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    dict = (NNObjDict*)nn_gcmem_protect(state, (NNObject*)nn_object_makedict(state));
    list = nn_value_asarray(args->thisval);
    for(i = 0; i < list->varray->listcount; i++)
    {
        nn_dict_setentry(dict, nn_value_makenumber(i), list->varray->listitems[i]);
    }
    return nn_value_fromobject(dict);
}

NNValue nn_objfnarray_iter(NNState* state, NNArguments* args)
{
    size_t index;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    list = nn_value_asarray(args->thisval);
    index = nn_value_asnumber(args->args[0]);
    if(((int)index > -1) && index < list->varray->listcount)
    {
        return list->varray->listitems[index];
    }
    return nn_value_makenull();
}

NNValue nn_objfnarray_itern(NNState* state, NNArguments* args)
{
    size_t index;
    NNObjArray* list;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    list = nn_value_asarray(args->thisval);
    if(nn_value_isnull(args->args[0]))
    {
        if(list->varray->listcount == 0)
        {
            return nn_value_makebool(false);
        }
        return nn_value_makenumber(0);
    }
    if(!nn_value_isnumber(args->args[0]))
    {
        NEON_RETURNERROR("lists are numerically indexed");
    }
    index = nn_value_asnumber(args->args[0]);
    if(index < list->varray->listcount - 1)
    {
        return nn_value_makenumber((double)index + 1);
    }
    return nn_value_makenull();
}

NNValue nn_objfnarray_each(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    NNValue callable;
    NNValue unused;
    NNObjArray* list;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    list = nn_value_asarray(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(arity > 0)
        {
            nestargs->varray->listitems[0] = list->varray->listitems[i];
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = nn_value_makenumber(i);
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &unused);
    }
    nn_vm_stackpop(state);
    return nn_value_makeempty();
}


NNValue nn_objfnarray_map(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    NNValue res;
    NNValue callable;
    NNObjArray* list;
    NNObjArray* nestargs;
    NNObjArray* resultlist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    list = nn_value_asarray(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    resultlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(!nn_value_isempty(list->varray->listitems[i]))
        {
            if(arity > 0)
            {
                nestargs->varray->listitems[0] = list->varray->listitems[i];
                if(arity > 1)
                {
                    nestargs->varray->listitems[1] = nn_value_makenumber(i);
                }
            }
            nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &res);
            nn_array_push(resultlist, res);
        }
        else
        {
            nn_array_push(resultlist, nn_value_makeempty());
        }
    }
    nn_vm_stackpop(state);
    return nn_value_fromobject(resultlist);
}


NNValue nn_objfnarray_filter(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    NNValue callable;
    NNValue result;
    NNObjArray* list;
    NNObjArray* nestargs;
    NNObjArray* resultlist;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    list = nn_value_asarray(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    resultlist = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(!nn_value_isempty(list->varray->listitems[i]))
        {
            if(arity > 0)
            {
                nestargs->varray->listitems[0] = list->varray->listitems[i];
                if(arity > 1)
                {
                    nestargs->varray->listitems[1] = nn_value_makenumber(i);
                }
            }
            nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &result);
            if(!nn_value_isfalse(result))
            {
                nn_array_push(resultlist, list->varray->listitems[i]);
            }
        }
    }
    nn_vm_stackpop(state);
    return nn_value_fromobject(resultlist);
}

NNValue nn_objfnarray_some(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    NNValue callable;
    NNValue result;
    NNObjArray* list;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    list = nn_value_asarray(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(!nn_value_isempty(list->varray->listitems[i]))
        {
            if(arity > 0)
            {
                nestargs->varray->listitems[0] = list->varray->listitems[i];
                if(arity > 1)
                {
                    nestargs->varray->listitems[1] = nn_value_makenumber(i);
                }
            }
            nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &result);
            if(!nn_value_isfalse(result))
            {
                nn_vm_stackpop(state);
                return nn_value_makebool(true);
            }
        }
    }
    nn_vm_stackpop(state);
    return nn_value_makebool(false);
}


NNValue nn_objfnarray_every(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    NNValue result;
    NNValue callable;
    NNObjArray* list;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    list = nn_value_asarray(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < list->varray->listcount; i++)
    {
        if(!nn_value_isempty(list->varray->listitems[i]))
        {
            if(arity > 0)
            {
                nestargs->varray->listitems[0] = list->varray->listitems[i];
                if(arity > 1)
                {
                    nestargs->varray->listitems[1] = nn_value_makenumber(i);
                }
            }
            nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &result);
            if(nn_value_isfalse(result))
            {
                nn_vm_stackpop(state);
                return nn_value_makebool(false);
            }
        }
    }
    nn_vm_stackpop(state);
    return nn_value_makebool(true);
}

NNValue nn_objfnarray_reduce(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    size_t startindex;
    NNValue callable;
    NNValue accumulator;
    NNObjArray* list;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    list = nn_value_asarray(args->thisval);
    callable = args->args[0];
    startindex = 0;
    accumulator = nn_value_makenull();
    if(args->count == 2)
    {
        accumulator = args->args[1];
    }
    if(nn_value_isnull(accumulator) && list->varray->listcount > 0)
    {
        accumulator = list->varray->listitems[0];
        startindex = 1;
    }
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = startindex; i < list->varray->listcount; i++)
    {
        if(!nn_value_isnull(list->varray->listitems[i]) && !nn_value_isempty(list->varray->listitems[i]))
        {
            if(arity > 0)
            {
                nestargs->varray->listitems[0] = accumulator;
                if(arity > 1)
                {
                    nestargs->varray->listitems[1] = list->varray->listitems[i];
                    if(arity > 2)
                    {
                        nestargs->varray->listitems[2] = nn_value_makenumber(i);
                        if(arity > 4)
                        {
                            nestargs->varray->listitems[3] = args->thisval;
                        }
                    }
                }
            }
            nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &accumulator);
        }
    }
    nn_vm_stackpop(state);
    return accumulator;
}

NNValue nn_objfnrange_lower(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makenumber(nn_value_asrange(args->thisval)->lower);
}

NNValue nn_objfnrange_upper(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makenumber(nn_value_asrange(args->thisval)->upper);
}

NNValue nn_objfnrange_range(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    return nn_value_makenumber(nn_value_asrange(args->thisval)->range);
}

NNValue nn_objfnrange_iter(NNState* state, NNArguments* args)
{
    int val;
    int index;
    NNObjRange* range;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    range = nn_value_asrange(args->thisval);
    index = nn_value_asnumber(args->args[0]);
    if(index >= 0 && index < range->range)
    {
        if(index == 0)
        {
            return nn_value_makenumber(range->lower);
        }
        if(range->lower > range->upper)
        {
            val = --range->lower;
        }
        else
        {
            val = ++range->lower;
        }
        return nn_value_makenumber(val);
    }
    return nn_value_makenull();
}

NNValue nn_objfnrange_itern(NNState* state, NNArguments* args)
{
    int index;
    NNObjRange* range;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    range = nn_value_asrange(args->thisval);
    if(nn_value_isnull(args->args[0]))
    {
        if(range->range == 0)
        {
            return nn_value_makenull();
        }
        return nn_value_makenumber(0);
    }
    if(!nn_value_isnumber(args->args[0]))
    {
        NEON_RETURNERROR("ranges are numerically indexed");
    }
    index = (int)nn_value_asnumber(args->args[0]) + 1;
    if(index < range->range)
    {
        return nn_value_makenumber(index);
    }
    return nn_value_makenull();
}

NNValue nn_objfnrange_loop(NNState* state, NNArguments* args)
{
    int i;
    int arity;
    NNValue callable;
    NNValue unused;
    NNObjRange* range;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    range = nn_value_asrange(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < range->range; i++)
    {
        if(arity > 0)
        {
            nestargs->varray->listitems[0] = nn_value_makenumber(i);
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = nn_value_makenumber(i);
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &unused);
    }
    nn_vm_stackpop(state);
    return nn_value_makeempty();
}

NNValue nn_objfnrange_expand(NNState* state, NNArguments* args)
{
    int i;
    NNValue val;
    NNObjRange* range;
    NNObjArray* oa;
    range = nn_value_asrange(args->thisval);
    oa = nn_object_makearray(state);
    for(i = 0; i < range->range; i++)
    {
        val = nn_value_makenumber(i);
        nn_array_push(oa, val);
    }
    return nn_value_fromobject(oa);
}

NNValue nn_objfnrange_constructor(NNState* state, NNArguments* args)
{
    int a;
    int b;
    NNObjRange* orng;
    a = nn_value_asnumber(args->args[0]);
    b = nn_value_asnumber(args->args[1]);
    orng = nn_object_makerange(state, a, b);
    return nn_value_fromobject(orng);
}

NNValue nn_objfnstring_utf8numbytes(NNState* state, NNArguments* args)
{
    int incode;
    int res;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    incode = nn_value_asnumber(args->args[0]);
    res = nn_util_utf8numbytes(incode);
    return nn_value_makenumber(res);
}

NNValue nn_objfnstring_utf8decode(NNState* state, NNArguments* args)
{
    int res;
    NNObjString* instr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    instr = nn_value_asstring(args->args[0]);
    res = nn_util_utf8decode((const uint8_t*)instr->sbuf->data, instr->sbuf->length);
    return nn_value_makenumber(res);
}

NNValue nn_objfnstring_utf8encode(NNState* state, NNArguments* args)
{
    int incode;
    size_t len;
    NNObjString* res;
    char* buf;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    incode = nn_value_asnumber(args->args[0]);
    buf = nn_util_utf8encode(state, incode, &len);
    res = nn_string_takelen(state, buf, len);
    return nn_value_fromobject(res);
}

NNValue nn_util_stringutf8chars(NNState* state, NNArguments* args, bool onlycodepoint)
{
    int cp;
    const char* cstr;
    NNObjArray* res;
    NNObjString* os;
    NNObjString* instr;
    utf8iterator_t iter;
    (void)state;
    instr = nn_value_asstring(args->thisval);
    res = nn_array_make(state);
    nn_utf8iter_init(&iter, instr->sbuf->data, instr->sbuf->length);
    while (nn_utf8iter_next(&iter))
    {
        cp = iter.codepoint;
        cstr = nn_utf8iter_getchar(&iter);
        if(onlycodepoint)
        {
            nn_array_push(res, nn_value_makenumber(cp));
        }
        else
        {
            os = nn_string_copylen(state, cstr, iter.charsize);
            nn_array_push(res, nn_value_fromobject(os));
        }
    }
    return nn_value_fromobject(res);
}

NNValue nn_objfnstring_utf8chars(NNState* state, NNArguments* args)
{
    return nn_util_stringutf8chars(state, args, false);
}

NNValue nn_objfnstring_utf8codepoints(NNState* state, NNArguments* args)
{
    return nn_util_stringutf8chars(state, args, true);
}


NNValue nn_objfnstring_fromcharcode(NNState* state, NNArguments* args)
{
    char ch;
    NNObjString* os;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    ch = nn_value_asnumber(args->args[0]);
    os = nn_string_copylen(state, &ch, 1);
    return nn_value_fromobject(os);
}

NNValue nn_objfnstring_constructor(NNState* state, NNArguments* args)
{
    NNObjString* os;
    NNArgCheck check;
    (void)args;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    os = nn_string_copylen(state, "", 0);
    return nn_value_fromobject(os);
}

NNValue nn_objfnstring_length(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    NNObjString* selfstr;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    return nn_value_makenumber(selfstr->sbuf->length);
}

NNValue nn_string_fromrange(NNState* state, const char* buf, int len)
{
    NNObjString* str;
    if(len <= 0)
    {
        return nn_value_fromobject(nn_string_copylen(state, "", 0));
    }
    str = nn_string_copylen(state, "", 0);
    dyn_strbuf_appendstrn(str->sbuf, buf, len);
    return nn_value_fromobject(str);
}

NNObjString* nn_string_substring(NNState* state, NNObjString* selfstr, size_t start, size_t end, bool likejs)
{
    size_t asz;
    size_t len;
    size_t tmp;
    size_t maxlen;
    char* raw;
    (void)likejs;
    maxlen = selfstr->sbuf->length;
    len = maxlen;
    if(end > maxlen)
    {
        tmp = start;
        start = end;
        end = tmp;
        len = maxlen;
    }
    if(end < start)
    {
        tmp = end;
        end = start;
        start = tmp;
        len = end;
    }
    len = (end - start);
    if(len > maxlen)
    {
        len = maxlen;
    }
    asz = ((end + 1) * sizeof(char));
    raw = (char*)nn_gcmem_allocate(state, sizeof(char), asz);
    memset(raw, 0, asz);
    memcpy(raw, selfstr->sbuf->data + start, len);
    return nn_string_takelen(state, raw, len);
}

NNValue nn_objfnstring_substring(NNState* state, NNArguments* args)
{
    size_t end;
    size_t start;
    size_t maxlen;
    NNObjString* nos;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    selfstr = nn_value_asstring(args->thisval);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    maxlen = selfstr->sbuf->length;
    end = maxlen;
    start = nn_value_asnumber(args->args[0]);
    if(args->count > 1)
    {
        NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
        end = nn_value_asnumber(args->args[1]);
    }
    nos = nn_string_substring(state, selfstr, start, end, true);
    return nn_value_fromobject(nos);
}

NNValue nn_objfnstring_charcodeat(NNState* state, NNArguments* args)
{
    int ch;
    int idx;
    int selflen;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    selfstr = nn_value_asstring(args->thisval);
    idx = nn_value_asnumber(args->args[0]);
    selflen = (int)selfstr->sbuf->length;
    if((idx < 0) || (idx >= selflen))
    {
        ch = -1;
    }
    else
    {
        ch = selfstr->sbuf->data[idx];
    }
    return nn_value_makenumber(ch);
}

NNValue nn_objfnstring_charat(NNState* state, NNArguments* args)
{
    char ch;
    int idx;
    int selflen;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    selfstr = nn_value_asstring(args->thisval);
    idx = nn_value_asnumber(args->args[0]);
    selflen = (int)selfstr->sbuf->length;
    if((idx < 0) || (idx >= selflen))
    {
        return nn_value_fromobject(nn_string_copylen(state, "", 0));
    }
    else
    {
        ch = selfstr->sbuf->data[idx];
    }
    return nn_value_fromobject(nn_string_copylen(state, &ch, 1));
}

NNValue nn_objfnstring_upper(NNState* state, NNArguments* args)
{
    size_t slen;
    char* string;
    NNObjString* str;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    str = nn_value_asstring(args->thisval);
    slen = str->sbuf->length;
    string = nn_util_strtoupper(str->sbuf->data, slen);
    return nn_value_fromobject(nn_string_copylen(state, string, slen));
}

NNValue nn_objfnstring_lower(NNState* state, NNArguments* args)
{
    size_t slen;
    char* string;
    NNObjString* str;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    str = nn_value_asstring(args->thisval);
    slen = str->sbuf->length;
    string = nn_util_strtolower(str->sbuf->data, slen);
    return nn_value_fromobject(nn_string_copylen(state, string, slen));
}

NNValue nn_objfnstring_isalpha(NNState* state, NNArguments* args)
{
    size_t i;
    NNArgCheck check;
    NNObjString* selfstr;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    for(i = 0; i < selfstr->sbuf->length; i++)
    {
        if(!isalpha((unsigned char)selfstr->sbuf->data[i]))
        {
            return nn_value_makebool(false);
        }
    }
    return nn_value_makebool(selfstr->sbuf->length != 0);
}

NNValue nn_objfnstring_isalnum(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    for(i = 0; i < selfstr->sbuf->length; i++)
    {
        if(!isalnum((unsigned char)selfstr->sbuf->data[i]))
        {
            return nn_value_makebool(false);
        }
    }
    return nn_value_makebool(selfstr->sbuf->length != 0);
}

NNValue nn_objfnstring_isfloat(NNState* state, NNArguments* args)
{
    double f;
    char* p;
    NNObjString* selfstr;
    NNArgCheck check;
    (void)f;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    errno = 0;
    if(selfstr->sbuf->length ==0)
    {
        return nn_value_makebool(false);
    }
    f = strtod(selfstr->sbuf->data, &p);
    if(errno)
    {
        return nn_value_makebool(false);
    }
    else
    {
        if(*p == 0)
        {
            return nn_value_makebool(true);
        }
    }
    return nn_value_makebool(false);
}

NNValue nn_objfnstring_isnumber(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    for(i = 0; i < selfstr->sbuf->length; i++)
    {
        if(!isdigit((unsigned char)selfstr->sbuf->data[i]))
        {
            return nn_value_makebool(false);
        }
    }
    return nn_value_makebool(selfstr->sbuf->length != 0);
}

NNValue nn_objfnstring_islower(NNState* state, NNArguments* args)
{
    size_t i;
    bool alphafound;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    alphafound = false;
    for(i = 0; i < selfstr->sbuf->length; i++)
    {
        if(!alphafound && !isdigit(selfstr->sbuf->data[0]))
        {
            alphafound = true;
        }
        if(isupper(selfstr->sbuf->data[0]))
        {
            return nn_value_makebool(false);
        }
    }
    return nn_value_makebool(alphafound);
}

NNValue nn_objfnstring_isupper(NNState* state, NNArguments* args)
{
    size_t i;
    bool alphafound;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    alphafound = false;
    for(i = 0; i < selfstr->sbuf->length; i++)
    {
        if(!alphafound && !isdigit(selfstr->sbuf->data[0]))
        {
            alphafound = true;
        }
        if(islower(selfstr->sbuf->data[0]))
        {
            return nn_value_makebool(false);
        }
    }
    return nn_value_makebool(alphafound);
}

NNValue nn_objfnstring_isspace(NNState* state, NNArguments* args)
{
    size_t i;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    for(i = 0; i < selfstr->sbuf->length; i++)
    {
        if(!isspace((unsigned char)selfstr->sbuf->data[i]))
        {
            return nn_value_makebool(false);
        }
    }
    return nn_value_makebool(selfstr->sbuf->length != 0);
}

NNValue nn_objfnstring_trim(NNState* state, NNArguments* args)
{
    char trimmer;
    char* end;
    char* string;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    trimmer = '\0';
    if(args->count == 1)
    {
        trimmer = (char)nn_value_asstring(args->args[0])->sbuf->data[0];
    }
    selfstr = nn_value_asstring(args->thisval);
    string = selfstr->sbuf->data;
    end = NULL;
    /* Trim leading space*/
    if(trimmer == '\0')
    {
        while(isspace((unsigned char)*string))
        {
            string++;
        }
    }
    else
    {
        while(trimmer == *string)
        {
            string++;
        }
    }
    /* All spaces? */
    if(*string == 0)
    {
        return nn_value_fromobject(nn_string_copylen(state, "", 0));
    }
    /* Trim trailing space */
    end = string + strlen(string) - 1;
    if(trimmer == '\0')
    {
        while(end > string && isspace((unsigned char)*end))
        {
            end--;
        }
    }
    else
    {
        while(end > string && trimmer == *end)
        {
            end--;
        }
    }
    end[1] = '\0';
    return nn_value_fromobject(nn_string_copycstr(state, string));
}

NNValue nn_objfnstring_ltrim(NNState* state, NNArguments* args)
{
    char* end;
    char* string;
    char trimmer;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    trimmer = '\0';
    if(args->count == 1)
    {
        trimmer = (char)nn_value_asstring(args->args[0])->sbuf->data[0];
    }
    selfstr = nn_value_asstring(args->thisval);
    string = selfstr->sbuf->data;
    end = NULL;
    /* Trim leading space */
    if(trimmer == '\0')
    {
        while(isspace((unsigned char)*string))
        {
            string++;
        }
    }
    else
    {
        while(trimmer == *string)
        {
            string++;
        }
    }
    /* All spaces? */
    if(*string == 0)
    {
        return nn_value_fromobject(nn_string_copylen(state, "", 0));
    }
    end = string + strlen(string) - 1;
    end[1] = '\0';
    return nn_value_fromobject(nn_string_copycstr(state, string));
}

NNValue nn_objfnstring_rtrim(NNState* state, NNArguments* args)
{
    char* end;
    char* string;
    char trimmer;
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    trimmer = '\0';
    if(args->count == 1)
    {
        trimmer = (char)nn_value_asstring(args->args[0])->sbuf->data[0];
    }
    selfstr = nn_value_asstring(args->thisval);
    string = selfstr->sbuf->data;
    end = NULL;
    /* All spaces? */
    if(*string == 0)
    {
        return nn_value_fromobject(nn_string_copylen(state, "", 0));
    }
    end = string + strlen(string) - 1;
    if(trimmer == '\0')
    {
        while(end > string && isspace((unsigned char)*end))
        {
            end--;
        }
    }
    else
    {
        while(end > string && trimmer == *end)
        {
            end--;
        }
    }
    /* Write new null terminator character */
    end[1] = '\0';
    return nn_value_fromobject(nn_string_copycstr(state, string));
}


NNValue nn_objfnarray_constructor(NNState* state, NNArguments* args)
{
    int cnt;
    NNValue filler;
    NNObjArray* arr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    filler = nn_value_makeempty();
    if(args->count > 1)
    {
        filler = args->args[1];
    }
    cnt = nn_value_asnumber(args->args[0]);
    arr = nn_array_makefilled(state, cnt, filler);
    return nn_value_fromobject(arr);
}

NNValue nn_objfnarray_join(NNState* state, NNArguments* args)
{
    size_t i;
    size_t count;
    NNPrinter pr;
    NNValue vjoinee;
    NNObjArray* selfarr;
    NNObjString* joinee;
    NNValue* list;
    selfarr = nn_value_asarray(args->thisval);
    joinee = NULL;
    if(args->count > 0)
    {
        vjoinee = args->args[0];
        if(nn_value_isstring(vjoinee))
        {
            joinee = nn_value_asstring(vjoinee);
        }
        else
        {
            joinee = nn_value_tostring(state, vjoinee);
        }
    }
    list = selfarr->varray->listitems;
    count = selfarr->varray->listcount;
    if(count == 0)
    {
        return nn_value_fromobject(nn_string_copycstr(state, ""));
    }
    nn_printer_makestackstring(state, &pr);
    for(i = 0; i < count; i++)
    {
        nn_printer_printvalue(&pr, list[i], false, true);
        if((joinee != NULL) && ((i+1) < count))
        {
            nn_printer_writestringl(&pr, joinee->sbuf->data, joinee->sbuf->length);
        }
    }
    return nn_value_fromobject(nn_printer_takestring(&pr));
}

NNValue nn_objfnstring_indexof(NNState* state, NNArguments* args)
{
    int startindex;
    char* result;
    char* haystack;
    NNObjString* string;
    NNObjString* needle;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->thisval);
    needle = nn_value_asstring(args->args[0]);
    startindex = 0;
    if(args->count == 2)
    {
        NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
        startindex = nn_value_asnumber(args->args[1]);
    }
    if(string->sbuf->length > 0 && needle->sbuf->length > 0)
    {
        haystack = string->sbuf->data;
        result = strstr(haystack + startindex, needle->sbuf->data);
        if(result != NULL)
        {
            return nn_value_makenumber((int)(result - haystack));
        }
    }
    return nn_value_makenumber(-1);
}

NNValue nn_objfnstring_startswith(NNState* state, NNArguments* args)
{
    NNObjString* substr;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->thisval);
    substr = nn_value_asstring(args->args[0]);
    if(string->sbuf->length == 0 || substr->sbuf->length == 0 || substr->sbuf->length > string->sbuf->length)
    {
        return nn_value_makebool(false);
    }
    return nn_value_makebool(memcmp(substr->sbuf->data, string->sbuf->data, substr->sbuf->length) == 0);
}

NNValue nn_objfnstring_endswith(NNState* state, NNArguments* args)
{
    int difference;
    NNObjString* substr;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->thisval);
    substr = nn_value_asstring(args->args[0]);
    if(string->sbuf->length == 0 || substr->sbuf->length == 0 || substr->sbuf->length > string->sbuf->length)
    {
        return nn_value_makebool(false);
    }
    difference = string->sbuf->length - substr->sbuf->length;
    return nn_value_makebool(memcmp(substr->sbuf->data, string->sbuf->data + difference, substr->sbuf->length) == 0);
}

NNValue nn_objfnstring_count(NNState* state, NNArguments* args)
{
    int count;
    const char* tmp;
    NNObjString* substr;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->thisval);
    substr = nn_value_asstring(args->args[0]);
    if(substr->sbuf->length == 0 || string->sbuf->length == 0)
    {
        return nn_value_makenumber(0);
    }
    count = 0;
    tmp = string->sbuf->data;
    while((tmp = nn_util_utf8strstr(tmp, substr->sbuf->data)))
    {
        count++;
        tmp++;
    }
    return nn_value_makenumber(count);
}

NNValue nn_objfnstring_tonumber(NNState* state, NNArguments* args)
{
    NNObjString* selfstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    selfstr = nn_value_asstring(args->thisval);
    return nn_value_makenumber(strtod(selfstr->sbuf->data, NULL));
}

NNValue nn_objfnstring_isascii(NNState* state, NNArguments* args)
{
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    if(args->count == 1)
    {
        NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isbool);
    }
    string = nn_value_asstring(args->thisval);
    return nn_value_fromobject(string);
}

NNValue nn_objfnstring_tolist(NNState* state, NNArguments* args)
{
    size_t i;
    size_t end;
    size_t start;
    size_t length;
    NNObjArray* list;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    string = nn_value_asstring(args->thisval);
    list = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    length = string->sbuf->length;
    if(length > 0)
    {
        for(i = 0; i < length; i++)
        {
            start = i;
            end = i + 1;
            nn_array_push(list, nn_value_fromobject(nn_string_copylen(state, string->sbuf->data + start, (int)(end - start))));
        }
    }
    return nn_value_fromobject(list);
}

NNValue nn_objfnstring_lpad(NNState* state, NNArguments* args)
{
    size_t i;
    size_t width;
    size_t fillsize;
    size_t finalsize;
    size_t finalutf8size;
    char fillchar;
    char* str;
    char* fill;
    NNObjString* ofillstr;
    NNObjString* result;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    string = nn_value_asstring(args->thisval);
    width = nn_value_asnumber(args->args[0]);
    fillchar = ' ';
    if(args->count == 2)
    {
        ofillstr = nn_value_asstring(args->args[1]);
        fillchar = ofillstr->sbuf->data[0];
    }
    if(width <= string->sbuf->length)
    {
        return args->thisval;
    }
    fillsize = width - string->sbuf->length;
    fill = (char*)nn_gcmem_allocate(state, sizeof(char), (size_t)fillsize + 1);
    finalsize = string->sbuf->length + fillsize;
    finalutf8size = string->sbuf->length + fillsize;
    for(i = 0; i < fillsize; i++)
    {
        fill[i] = fillchar;
    }
    str = (char*)nn_gcmem_allocate(state, sizeof(char), (size_t)finalsize + 1);
    memcpy(str, fill, fillsize);
    memcpy(str + fillsize, string->sbuf->data, string->sbuf->length);
    str[finalsize] = '\0';
    nn_gcmem_freearray(state, sizeof(char), fill, fillsize + 1);
    result = nn_string_takelen(state, str, finalsize);
    result->sbuf->length = finalutf8size;
    result->sbuf->length = finalsize;
    return nn_value_fromobject(result);
}

NNValue nn_objfnstring_rpad(NNState* state, NNArguments* args)
{
    size_t i;
    size_t width;
    size_t fillsize;
    size_t finalsize;
    size_t finalutf8size;
    char fillchar;
    char* str;
    char* fill;
    NNObjString* ofillstr;
    NNObjString* string;
    NNObjString* result;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    string = nn_value_asstring(args->thisval);
    width = nn_value_asnumber(args->args[0]);
    fillchar = ' ';
    if(args->count == 2)
    {
        ofillstr = nn_value_asstring(args->args[1]);
        fillchar = ofillstr->sbuf->data[0];
    }
    if(width <= string->sbuf->length)
    {
        return args->thisval;
    }
    fillsize = width - string->sbuf->length;
    fill = (char*)nn_gcmem_allocate(state, sizeof(char), (size_t)fillsize + 1);
    finalsize = string->sbuf->length + fillsize;
    finalutf8size = string->sbuf->length + fillsize;
    for(i = 0; i < fillsize; i++)
    {
        fill[i] = fillchar;
    }
    str = (char*)nn_gcmem_allocate(state, sizeof(char), (size_t)finalsize + 1);
    memcpy(str, string->sbuf->data, string->sbuf->length);
    memcpy(str + string->sbuf->length, fill, fillsize);
    str[finalsize] = '\0';
    nn_gcmem_freearray(state, sizeof(char), fill, fillsize + 1);
    result = nn_string_takelen(state, str, finalsize);
    result->sbuf->length = finalutf8size;
    result->sbuf->length = finalsize;
    return nn_value_fromobject(result);
}

NNValue nn_objfnstring_split(NNState* state, NNArguments* args)
{
    size_t i;
    size_t end;
    size_t start;
    size_t length;
    NNObjArray* list;
    NNObjString* string;
    NNObjString* delimeter;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 1, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->thisval);
    delimeter = nn_value_asstring(args->args[0]);
    /* empty string matches empty string to empty list */
    if(((string->sbuf->length == 0) && (delimeter->sbuf->length == 0)) || (string->sbuf->length == 0) || (delimeter->sbuf->length == 0))
    {
        return nn_value_fromobject(nn_object_makearray(state));
    }
    list = (NNObjArray*)nn_gcmem_protect(state, (NNObject*)nn_object_makearray(state));
    if(delimeter->sbuf->length > 0)
    {
        start = 0;
        for(i = 0; i <= string->sbuf->length; i++)
        {
            /* match found. */
            if(memcmp(string->sbuf->data + i, delimeter->sbuf->data, delimeter->sbuf->length) == 0 || i == string->sbuf->length)
            {
                nn_array_push(list, nn_value_fromobject(nn_string_copylen(state, string->sbuf->data + start, i - start)));
                i += delimeter->sbuf->length - 1;
                start = i + 1;
            }
        }
    }
    else
    {
        length = string->sbuf->length;
        for(i = 0; i < length; i++)
        {
            start = i;
            end = i + 1;
            nn_array_push(list, nn_value_fromobject(nn_string_copylen(state, string->sbuf->data + start, (int)(end - start))));
        }
    }
    return nn_value_fromobject(list);
}


NNValue nn_objfnstring_replace(NNState* state, NNArguments* args)
{
    size_t i;
    size_t totallength;
    StringBuffer* result;
    NNObjString* substr;
    NNObjString* string;
    NNObjString* repsubstr;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 2, 3);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isstring);
    string = nn_value_asstring(args->thisval);
    substr = nn_value_asstring(args->args[0]);
    repsubstr = nn_value_asstring(args->args[1]);
    if((string->sbuf->length == 0 && substr->sbuf->length == 0) || string->sbuf->length == 0 || substr->sbuf->length == 0)
    {
        return nn_value_fromobject(nn_string_copylen(state, string->sbuf->data, string->sbuf->length));
    }
    result = dyn_strbuf_makeempty(0);
    totallength = 0;
    for(i = 0; i < string->sbuf->length; i++)
    {
        if(memcmp(string->sbuf->data + i, substr->sbuf->data, substr->sbuf->length) == 0)
        {
            if(substr->sbuf->length > 0)
            {
                dyn_strbuf_appendstrn(result, repsubstr->sbuf->data, repsubstr->sbuf->length);
            }
            i += substr->sbuf->length - 1;
            totallength += repsubstr->sbuf->length;
        }
        else
        {
            dyn_strbuf_appendchar(result, string->sbuf->data[i]);
            totallength++;
        }
    }
    return nn_value_fromobject(nn_string_makefromstrbuf(state, result, nn_util_hashstring(result->data, result->length)));
}

NNValue nn_objfnstring_iter(NNState* state, NNArguments* args)
{
    size_t index;
    size_t length;
    NNObjString* string;
    NNObjString* result;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    string = nn_value_asstring(args->thisval);
    length = string->sbuf->length;
    index = nn_value_asnumber(args->args[0]);
    if(((int)index > -1) && (index < length))
    {
        result = nn_string_copylen(state, &string->sbuf->data[index], 1);
        return nn_value_fromobject(result);
    }
    return nn_value_makenull();
}

NNValue nn_objfnstring_itern(NNState* state, NNArguments* args)
{
    size_t index;
    size_t length;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    string = nn_value_asstring(args->thisval);
    length = string->sbuf->length;
    if(nn_value_isnull(args->args[0]))
    {
        if(length == 0)
        {
            return nn_value_makebool(false);
        }
        return nn_value_makenumber(0);
    }
    if(!nn_value_isnumber(args->args[0]))
    {
        NEON_RETURNERROR("strings are numerically indexed");
    }
    index = nn_value_asnumber(args->args[0]);
    if(index < length - 1)
    {
        return nn_value_makenumber((double)index + 1);
    }
    return nn_value_makenull();
}

NNValue nn_objfnstring_each(NNState* state, NNArguments* args)
{
    size_t i;
    size_t arity;
    NNValue callable;
    NNValue unused;
    NNObjString* string;
    NNObjArray* nestargs;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_iscallable);
    string = nn_value_asstring(args->thisval);
    callable = args->args[0];
    nestargs = nn_object_makearray(state);
    nn_vm_stackpush(state, nn_value_fromobject(nestargs));
    arity = nn_nestcall_prepare(state, callable, args->thisval, nestargs);
    for(i = 0; i < string->sbuf->length; i++)
    {
        if(arity > 0)
        {
            nestargs->varray->listitems[0] = nn_value_fromobject(nn_string_copylen(state, string->sbuf->data + i, 1));
            if(arity > 1)
            {
                nestargs->varray->listitems[1] = nn_value_makenumber(i);
            }
        }
        nn_nestcall_callfunction(state, callable, args->thisval, nestargs, &unused);
    }
    /* pop the argument list */
    nn_vm_stackpop(state);
    return nn_value_makeempty();
}

NNValue nn_objfnobject_dump(NNState* state, NNArguments* args)
{
    NNValue v;
    NNPrinter pr;
    NNObjString* os;
    v = args->thisval;
    nn_printer_makestackstring(state, &pr);
    nn_printer_printvalue(&pr, v, true, false);
    os = nn_printer_takestring(&pr);
    return nn_value_fromobject(os);
}

NNValue nn_objfnobject_tostring(NNState* state, NNArguments* args)
{
    NNValue v;
    NNPrinter pr;
    NNObjString* os;
    v = args->thisval;
    nn_printer_makestackstring(state, &pr);
    nn_printer_printvalue(&pr, v, false, true);
    os = nn_printer_takestring(&pr);
    return nn_value_fromobject(os);
}

NNValue nn_objfnobject_typename(NNState* state, NNArguments* args)
{
    NNValue v;
    NNObjString* os;
    v = args->args[0];
    os = nn_string_copycstr(state, nn_value_typename(v));
    return nn_value_fromobject(os);
}

NNValue nn_objfnobject_isstring(NNState* state, NNArguments* args)
{
    NNValue v;
    (void)state;
    v = args->thisval;
    return nn_value_makebool(nn_value_isstring(v));
}

NNValue nn_objfnobject_isarray(NNState* state, NNArguments* args)
{
    NNValue v;
    (void)state;
    v = args->thisval;
    return nn_value_makebool(nn_value_isarray(v));
}

NNValue nn_objfnobject_iscallable(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return (nn_value_makebool(
        nn_value_isclass(selfval) ||
        nn_value_isfuncscript(selfval) ||
        nn_value_isfuncclosure(selfval) ||
        nn_value_isfuncbound(selfval) ||
        nn_value_isfuncnative(selfval)
    ));
}

NNValue nn_objfnobject_isbool(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isbool(selfval));
}

NNValue nn_objfnobject_isnumber(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isnumber(selfval));
}

NNValue nn_objfnobject_isint(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isnumber(selfval) && (((int)nn_value_asnumber(selfval)) == nn_value_asnumber(selfval)));
}

NNValue nn_objfnobject_isdict(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isdict(selfval));
}

NNValue nn_objfnobject_isobject(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isobject(selfval));
}

NNValue nn_objfnobject_isfunction(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(
        nn_value_isfuncscript(selfval) ||
        nn_value_isfuncclosure(selfval) ||
        nn_value_isfuncbound(selfval) ||
        nn_value_isfuncnative(selfval)
    );
}

NNValue nn_objfnobject_isiterable(NNState* state, NNArguments* args)
{
    bool isiterable;
    NNValue dummy;
    NNObjClass* klass;
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    isiterable = nn_value_isarray(selfval) || nn_value_isdict(selfval) || nn_value_isstring(selfval);
    if(!isiterable && nn_value_isinstance(selfval))
    {
        klass = nn_value_asinstance(selfval)->klass;
        isiterable = nn_table_get(klass->methods, nn_value_fromobject(nn_string_copycstr(state, "@iter")), &dummy)
            && nn_table_get(klass->methods, nn_value_fromobject(nn_string_copycstr(state, "@itern")), &dummy);
    }
    return nn_value_makebool(isiterable);
}

NNValue nn_objfnobject_isclass(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isclass(selfval));
}

NNValue nn_objfnobject_isfile(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isfile(selfval));
}

NNValue nn_objfnobject_isinstance(NNState* state, NNArguments* args)
{
    NNValue selfval;
    (void)state;
    selfval = args->thisval;
    return nn_value_makebool(nn_value_isinstance(selfval));
}


NNObjString* nn_util_numbertobinstring(NNState* state, long n)
{
    int i;
    int rem;
    int count;
    int length;
    long j;
    /* assume maximum of 1024 bits */
    char str[1024];
    char newstr[1027];
    count = 0;
    j = n;
    if(j == 0)
    {
        str[count++] = '0';
    }
    while(j != 0)
    {
        rem = abs((int)(j % 2));
        j /= 2;
        if(rem == 1)
        {
            str[count] = '1';
        }
        else
        {
            str[count] = '0';
        }
        count++;
    }
    /* assume maximum of 1024 bits + 0b (indicator) + sign (-). */
    length = 0;
    if(n < 0)
    {
        newstr[length++] = '-';
    }
    newstr[length++] = '0';
    newstr[length++] = 'b';
    for(i = count - 1; i >= 0; i--)
    {
        newstr[length++] = str[i];
    }
    newstr[length++] = 0;
    return nn_string_copylen(state, newstr, length);
    /*
    //  // To store the binary number
    //  int64_t number = 0;
    //  int cnt = 0;
    //  while (n != 0) {
    //    int64_t rem = n % 2;
    //    int64_t c = (int64_t) pow(10, cnt);
    //    number += rem * c;
    //    n /= 2;
    //
    //    // Count used to store exponent value
    //    cnt++;
    //  }
    //
    //  char str[67]; // assume maximum of 64 bits + 2 binary indicators (0b)
    //  int length = sprintf(str, "0b%lld", number);
    //
    //  return nn_string_copylen(state, str, length);
    */
}

NNObjString* nn_util_numbertooctstring(NNState* state, int64_t n, bool numeric)
{
    int length;
    /* assume maximum of 64 bits + 2 octal indicators (0c) */
    char str[66];
    length = sprintf(str, numeric ? "0c%lo" : "%lo", n);
    return nn_string_copylen(state, str, length);
}

NNObjString* nn_util_numbertohexstring(NNState* state, int64_t n, bool numeric)
{
    int length;
    /* assume maximum of 64 bits + 2 hex indicators (0x) */
    char str[66];
    length = sprintf(str, numeric ? "0x%lx" : "%lx", n);
    return nn_string_copylen(state, str, length);
}

NNValue nn_objfnnumber_tobinstring(NNState* state, NNArguments* args)
{
    return nn_value_fromobject(nn_util_numbertobinstring(state, nn_value_asnumber(args->thisval)));
}

NNValue nn_objfnnumber_tooctstring(NNState* state, NNArguments* args)
{
    return nn_value_fromobject(nn_util_numbertooctstring(state, nn_value_asnumber(args->thisval), false));
}

NNValue nn_objfnnumber_tohexstring(NNState* state, NNArguments* args)
{
    return nn_value_fromobject(nn_util_numbertohexstring(state, nn_value_asnumber(args->thisval), false));
}

NNValue nn_objfnmath_abs(NNState* state, NNArguments* args)
{
    (void)state;
    return nn_value_makenumber(fabs(nn_value_asnumber(args->thisval)));
}

NNValue nn_objfnmath_round(NNState* state, NNArguments* args)
{
    (void)state;
    return nn_value_makenumber(round(nn_value_asnumber(args->thisval)));
}

NNValue nn_objfnmath_sqrt(NNState* state, NNArguments* args)
{
    (void)state;
    return nn_value_makenumber(sqrt(nn_value_asnumber(args->thisval)));
}

NNValue nn_objfnmath_ceil(NNState* state, NNArguments* args)
{
    (void)state;
    return nn_value_makenumber(ceil(nn_value_asnumber(args->thisval)));
}

NNValue nn_objfnmath_floor(NNState* state, NNArguments* args)
{
    (void)state;
    return nn_value_makenumber(floor(nn_value_asnumber(args->thisval)));
}

NNValue nn_objfnmath_min(NNState* state, NNArguments* args)
{
    double b;
    double x;
    double y;
    (void)state;
    x = nn_value_asnumber(args->args[0]);
    y = nn_value_asnumber(args->args[1]);
    b = (x < y) ? x : y;
    return nn_value_makenumber(b);
}

NNValue nn_objfnprocess_scriptlocation(NNState* state, NNArguments* args)
{
    return nn_value_fromobject(state->clidirectory);
}

void nn_state_initbuiltinmethods(NNState* state)
{
    {
        nn_class_setstaticpropertycstr(state->classprimprocess, "env", nn_value_fromobject(state->envdict));
        nn_class_defstaticnativemethod(state, state->classprimprocess, "directory", nn_objfnprocess_scriptlocation);
    }
    {
        nn_class_defstaticnativemethod(state, state->classprimobject, "typename", nn_objfnobject_typename);
        nn_class_defnativemethod(state, state->classprimobject, "dump", nn_objfnobject_dump);
        nn_class_defnativemethod(state, state->classprimobject, "toString", nn_objfnobject_tostring);
        nn_class_defnativemethod(state, state->classprimobject, "isArray", nn_objfnobject_isarray);        
        nn_class_defnativemethod(state, state->classprimobject, "isString", nn_objfnobject_isstring);
        nn_class_defnativemethod(state, state->classprimobject, "isCallable", nn_objfnobject_iscallable);
        nn_class_defnativemethod(state, state->classprimobject, "isBool", nn_objfnobject_isbool);
        nn_class_defnativemethod(state, state->classprimobject, "isNumber", nn_objfnobject_isnumber);
        nn_class_defnativemethod(state, state->classprimobject, "isInt", nn_objfnobject_isint);
        nn_class_defnativemethod(state, state->classprimobject, "isDict", nn_objfnobject_isdict);
        nn_class_defnativemethod(state, state->classprimobject, "isObject", nn_objfnobject_isobject);
        nn_class_defnativemethod(state, state->classprimobject, "isFunction", nn_objfnobject_isfunction);
        nn_class_defnativemethod(state, state->classprimobject, "isIterable", nn_objfnobject_isiterable);
        nn_class_defnativemethod(state, state->classprimobject, "isClass", nn_objfnobject_isclass);
        nn_class_defnativemethod(state, state->classprimobject, "isFile", nn_objfnobject_isfile);
        nn_class_defnativemethod(state, state->classprimobject, "isInstance", nn_objfnobject_isinstance);

    }
    
    {
        nn_class_defnativemethod(state, state->classprimnumber, "toHexString", nn_objfnnumber_tohexstring);
        nn_class_defnativemethod(state, state->classprimnumber, "toOctString", nn_objfnnumber_tooctstring);
        nn_class_defnativemethod(state, state->classprimnumber, "toBinString", nn_objfnnumber_tobinstring);
    }
    {
        nn_class_defnativeconstructor(state, state->classprimstring, nn_objfnstring_constructor);
        nn_class_defstaticnativemethod(state, state->classprimstring, "fromCharCode", nn_objfnstring_fromcharcode);
        nn_class_defstaticnativemethod(state, state->classprimstring, "utf8Decode", nn_objfnstring_utf8decode);
        nn_class_defstaticnativemethod(state, state->classprimstring, "utf8Encode", nn_objfnstring_utf8encode);
        nn_class_defstaticnativemethod(state, state->classprimstring, "utf8NumBytes", nn_objfnstring_utf8numbytes);

        nn_class_defnativemethod(state, state->classprimstring, "utf8Chars", nn_objfnstring_utf8chars);
        nn_class_defnativemethod(state, state->classprimstring, "utf8Codepoints", nn_objfnstring_utf8codepoints);
        nn_class_defnativemethod(state, state->classprimstring, "utf8Bytes", nn_objfnstring_utf8codepoints);
        nn_class_defcallablefield(state, state->classprimstring, "length", nn_objfnstring_length);
        nn_class_defnativemethod(state, state->classprimstring, "@iter", nn_objfnstring_iter);
        nn_class_defnativemethod(state, state->classprimstring, "@itern", nn_objfnstring_itern);
        nn_class_defnativemethod(state, state->classprimstring, "size", nn_objfnstring_length);
        nn_class_defnativemethod(state, state->classprimstring, "substr", nn_objfnstring_substring);
        nn_class_defnativemethod(state, state->classprimstring, "substring", nn_objfnstring_substring);
        nn_class_defnativemethod(state, state->classprimstring, "charCodeAt", nn_objfnstring_charcodeat);
        nn_class_defnativemethod(state, state->classprimstring, "charAt", nn_objfnstring_charat);
        nn_class_defnativemethod(state, state->classprimstring, "upper", nn_objfnstring_upper);
        nn_class_defnativemethod(state, state->classprimstring, "lower", nn_objfnstring_lower);
        nn_class_defnativemethod(state, state->classprimstring, "trim", nn_objfnstring_trim);
        nn_class_defnativemethod(state, state->classprimstring, "ltrim", nn_objfnstring_ltrim);
        nn_class_defnativemethod(state, state->classprimstring, "rtrim", nn_objfnstring_rtrim);
        nn_class_defnativemethod(state, state->classprimstring, "split", nn_objfnstring_split);
        nn_class_defnativemethod(state, state->classprimstring, "indexOf", nn_objfnstring_indexof);
        nn_class_defnativemethod(state, state->classprimstring, "count", nn_objfnstring_count);
        nn_class_defnativemethod(state, state->classprimstring, "toNumber", nn_objfnstring_tonumber);
        nn_class_defnativemethod(state, state->classprimstring, "toList", nn_objfnstring_tolist);
        nn_class_defnativemethod(state, state->classprimstring, "lpad", nn_objfnstring_lpad);
        nn_class_defnativemethod(state, state->classprimstring, "rpad", nn_objfnstring_rpad);
        nn_class_defnativemethod(state, state->classprimstring, "replace", nn_objfnstring_replace);
        nn_class_defnativemethod(state, state->classprimstring, "each", nn_objfnstring_each);
        nn_class_defnativemethod(state, state->classprimstring, "startswith", nn_objfnstring_startswith);
        nn_class_defnativemethod(state, state->classprimstring, "endswith", nn_objfnstring_endswith);
        nn_class_defnativemethod(state, state->classprimstring, "isAscii", nn_objfnstring_isascii);
        nn_class_defnativemethod(state, state->classprimstring, "isAlpha", nn_objfnstring_isalpha);
        nn_class_defnativemethod(state, state->classprimstring, "isAlnum", nn_objfnstring_isalnum);
        nn_class_defnativemethod(state, state->classprimstring, "isNumber", nn_objfnstring_isnumber);
        nn_class_defnativemethod(state, state->classprimstring, "isFloat", nn_objfnstring_isfloat);
        nn_class_defnativemethod(state, state->classprimstring, "isLower", nn_objfnstring_islower);
        nn_class_defnativemethod(state, state->classprimstring, "isUpper", nn_objfnstring_isupper);
        nn_class_defnativemethod(state, state->classprimstring, "isSpace", nn_objfnstring_isspace);
        
    }
    {
        #if 1
        nn_class_defnativeconstructor(state, state->classprimarray, nn_objfnarray_constructor);
        #endif
        nn_class_defcallablefield(state, state->classprimarray, "length", nn_objfnarray_length);
        nn_class_defnativemethod(state, state->classprimarray, "size", nn_objfnarray_length);
        nn_class_defnativemethod(state, state->classprimarray, "join", nn_objfnarray_join);
        nn_class_defnativemethod(state, state->classprimarray, "append", nn_objfnarray_append);
        nn_class_defnativemethod(state, state->classprimarray, "push", nn_objfnarray_append);
        nn_class_defnativemethod(state, state->classprimarray, "clear", nn_objfnarray_clear);
        nn_class_defnativemethod(state, state->classprimarray, "clone", nn_objfnarray_clone);
        nn_class_defnativemethod(state, state->classprimarray, "count", nn_objfnarray_count);
        nn_class_defnativemethod(state, state->classprimarray, "extend", nn_objfnarray_extend);
        nn_class_defnativemethod(state, state->classprimarray, "indexOf", nn_objfnarray_indexof);
        nn_class_defnativemethod(state, state->classprimarray, "insert", nn_objfnarray_insert);
        nn_class_defnativemethod(state, state->classprimarray, "pop", nn_objfnarray_pop);
        nn_class_defnativemethod(state, state->classprimarray, "shift", nn_objfnarray_shift);
        nn_class_defnativemethod(state, state->classprimarray, "removeAt", nn_objfnarray_removeat);
        nn_class_defnativemethod(state, state->classprimarray, "remove", nn_objfnarray_remove);
        nn_class_defnativemethod(state, state->classprimarray, "reverse", nn_objfnarray_reverse);
        nn_class_defnativemethod(state, state->classprimarray, "sort", nn_objfnarray_sort);
        nn_class_defnativemethod(state, state->classprimarray, "contains", nn_objfnarray_contains);
        nn_class_defnativemethod(state, state->classprimarray, "delete", nn_objfnarray_delete);
        nn_class_defnativemethod(state, state->classprimarray, "first", nn_objfnarray_first);
        nn_class_defnativemethod(state, state->classprimarray, "last", nn_objfnarray_last);
        nn_class_defnativemethod(state, state->classprimarray, "isEmpty", nn_objfnarray_isempty);
        nn_class_defnativemethod(state, state->classprimarray, "take", nn_objfnarray_take);
        nn_class_defnativemethod(state, state->classprimarray, "get", nn_objfnarray_get);
        nn_class_defnativemethod(state, state->classprimarray, "compact", nn_objfnarray_compact);
        nn_class_defnativemethod(state, state->classprimarray, "unique", nn_objfnarray_unique);
        nn_class_defnativemethod(state, state->classprimarray, "zip", nn_objfnarray_zip);
        nn_class_defnativemethod(state, state->classprimarray, "zipFrom", nn_objfnarray_zipfrom);
        nn_class_defnativemethod(state, state->classprimarray, "toDict", nn_objfnarray_todict);
        nn_class_defnativemethod(state, state->classprimarray, "each", nn_objfnarray_each);
        nn_class_defnativemethod(state, state->classprimarray, "map", nn_objfnarray_map);
        nn_class_defnativemethod(state, state->classprimarray, "filter", nn_objfnarray_filter);
        nn_class_defnativemethod(state, state->classprimarray, "some", nn_objfnarray_some);
        nn_class_defnativemethod(state, state->classprimarray, "every", nn_objfnarray_every);
        nn_class_defnativemethod(state, state->classprimarray, "reduce", nn_objfnarray_reduce);
        nn_class_defnativemethod(state, state->classprimarray, "@iter", nn_objfnarray_iter);
        nn_class_defnativemethod(state, state->classprimarray, "@itern", nn_objfnarray_itern);
    }
    {
        #if 0
        nn_class_defnativeconstructor(state, state->classprimdict, nn_objfndict_constructor);
        nn_class_defstaticnativemethod(state, state->classprimdict, "keys", nn_objfndict_keys);
        #endif
        nn_class_defnativemethod(state, state->classprimdict, "keys", nn_objfndict_keys);
        nn_class_defnativemethod(state, state->classprimdict, "size", nn_objfndict_length);
        nn_class_defnativemethod(state, state->classprimdict, "add", nn_objfndict_add);
        nn_class_defnativemethod(state, state->classprimdict, "set", nn_objfndict_set);
        nn_class_defnativemethod(state, state->classprimdict, "clear", nn_objfndict_clear);
        nn_class_defnativemethod(state, state->classprimdict, "clone", nn_objfndict_clone);
        nn_class_defnativemethod(state, state->classprimdict, "compact", nn_objfndict_compact);
        nn_class_defnativemethod(state, state->classprimdict, "contains", nn_objfndict_contains);
        nn_class_defnativemethod(state, state->classprimdict, "extend", nn_objfndict_extend);
        nn_class_defnativemethod(state, state->classprimdict, "get", nn_objfndict_get);
        nn_class_defnativemethod(state, state->classprimdict, "values", nn_objfndict_values);
        nn_class_defnativemethod(state, state->classprimdict, "remove", nn_objfndict_remove);
        nn_class_defnativemethod(state, state->classprimdict, "isEmpty", nn_objfndict_isempty);
        nn_class_defnativemethod(state, state->classprimdict, "findKey", nn_objfndict_findkey);
        nn_class_defnativemethod(state, state->classprimdict, "toList", nn_objfndict_tolist);
        nn_class_defnativemethod(state, state->classprimdict, "each", nn_objfndict_each);
        nn_class_defnativemethod(state, state->classprimdict, "filter", nn_objfndict_filter);
        nn_class_defnativemethod(state, state->classprimdict, "some", nn_objfndict_some);
        nn_class_defnativemethod(state, state->classprimdict, "every", nn_objfndict_every);
        nn_class_defnativemethod(state, state->classprimdict, "reduce", nn_objfndict_reduce);
        nn_class_defnativemethod(state, state->classprimdict, "@iter", nn_objfndict_iter);
        nn_class_defnativemethod(state, state->classprimdict, "@itern", nn_objfndict_itern);
    }
    {
        nn_class_defnativeconstructor(state, state->classprimfile, nn_objfnfile_constructor);
        nn_class_defstaticnativemethod(state, state->classprimfile, "exists", nn_objfnfile_exists);
        nn_class_defnativemethod(state, state->classprimfile, "close", nn_objfnfile_close);
        nn_class_defnativemethod(state, state->classprimfile, "open", nn_objfnfile_open);
        nn_class_defnativemethod(state, state->classprimfile, "read", nn_objfnfile_read);
        nn_class_defnativemethod(state, state->classprimfile, "get", nn_objfnfile_get);
        nn_class_defnativemethod(state, state->classprimfile, "gets", nn_objfnfile_gets);
        nn_class_defnativemethod(state, state->classprimfile, "write", nn_objfnfile_write);
        nn_class_defnativemethod(state, state->classprimfile, "puts", nn_objfnfile_puts);
        nn_class_defnativemethod(state, state->classprimfile, "printf", nn_objfnfile_printf);
        nn_class_defnativemethod(state, state->classprimfile, "number", nn_objfnfile_number);
        nn_class_defnativemethod(state, state->classprimfile, "isTTY", nn_objfnfile_istty);
        nn_class_defnativemethod(state, state->classprimfile, "isOpen", nn_objfnfile_isopen);
        nn_class_defnativemethod(state, state->classprimfile, "isClosed", nn_objfnfile_isclosed);
        nn_class_defnativemethod(state, state->classprimfile, "flush", nn_objfnfile_flush);
        nn_class_defnativemethod(state, state->classprimfile, "stats", nn_objfnfile_stats);
        nn_class_defnativemethod(state, state->classprimfile, "path", nn_objfnfile_path);
        nn_class_defnativemethod(state, state->classprimfile, "seek", nn_objfnfile_seek);
        nn_class_defnativemethod(state, state->classprimfile, "tell", nn_objfnfile_tell);
        nn_class_defnativemethod(state, state->classprimfile, "mode", nn_objfnfile_mode);
        nn_class_defnativemethod(state, state->classprimfile, "name", nn_objfnfile_name);
    }
    {
        nn_class_defnativeconstructor(state, state->classprimrange, nn_objfnrange_constructor);
        nn_class_defnativemethod(state, state->classprimrange, "lower", nn_objfnrange_lower);
        nn_class_defnativemethod(state, state->classprimrange, "upper", nn_objfnrange_upper);
        nn_class_defnativemethod(state, state->classprimrange, "range", nn_objfnrange_range);
        nn_class_defnativemethod(state, state->classprimrange, "loop", nn_objfnrange_loop);
        nn_class_defnativemethod(state, state->classprimrange, "expand", nn_objfnrange_expand);
        nn_class_defnativemethod(state, state->classprimrange, "toArray", nn_objfnrange_expand);
        nn_class_defnativemethod(state, state->classprimrange, "@iter", nn_objfnrange_iter);
        nn_class_defnativemethod(state, state->classprimrange, "@itern", nn_objfnrange_itern);
    }
    {
        nn_class_defstaticnativemethod(state, state->classprimmath, "abs", nn_objfnmath_abs);
        nn_class_defstaticnativemethod(state, state->classprimmath, "round", nn_objfnmath_round);
        nn_class_defstaticnativemethod(state, state->classprimmath, "sqrt", nn_objfnmath_sqrt);
        nn_class_defstaticnativemethod(state, state->classprimmath, "ceil", nn_objfnmath_ceil);
        nn_class_defstaticnativemethod(state, state->classprimmath, "floor", nn_objfnmath_floor);
        nn_class_defstaticnativemethod(state, state->classprimmath, "min", nn_objfnmath_min);
    }
}

NNValue nn_nativefn_time(NNState* state, NNArguments* args)
{
    struct timeval tv;
    NNArgCheck check;
    (void)args;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    osfn_gettimeofday(&tv, NULL);
    return nn_value_makenumber((double)tv.tv_sec + ((double)tv.tv_usec / 10000000));
}

NNValue nn_nativefn_microtime(NNState* state, NNArguments* args)
{
    struct timeval tv;
    NNArgCheck check;
    (void)args;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 0);
    osfn_gettimeofday(&tv, NULL);
    return nn_value_makenumber((1000000 * (double)tv.tv_sec) + ((double)tv.tv_usec / 10));
}

NNValue nn_nativefn_id(NNState* state, NNArguments* args)
{
    NNValue val;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    val = args->args[0];
    return nn_value_makenumber(*(long*)&val);
}

NNValue nn_nativefn_int(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 1);
    if(args->count == 0)
    {
        return nn_value_makenumber(0);
    }
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    return nn_value_makenumber((double)((int)nn_value_asnumber(args->args[0])));
}

NNValue nn_nativefn_chr(NNState* state, NNArguments* args)
{
    size_t len;
    char* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
    string = nn_util_utf8encode(state, (int)nn_value_asnumber(args->args[0]), &len);
    return nn_value_fromobject(nn_string_takecstr(state, string));
}

NNValue nn_nativefn_ord(NNState* state, NNArguments* args)
{
    int ord;
    int length;
    NNObjString* string;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    string = nn_value_asstring(args->args[0]);
    length = string->sbuf->length;
    if(length > 1)
    {
        NEON_RETURNERROR("ord() expects character as argument, string given");
    }
    ord = (int)string->sbuf->data[0];
    if(ord < 0)
    {
        ord += 256;
    }
    return nn_value_makenumber(ord);
}

void nn_util_mtseed(uint32_t seed, uint32_t* binst, uint32_t* index)
{
    uint32_t i;
    binst[0] = seed;
    for(i = 1; i < MT_STATE_SIZE; i++)
    {
        binst[i] = (uint32_t)(1812433253UL * (binst[i - 1] ^ (binst[i - 1] >> 30)) + i);
    }
    *index = MT_STATE_SIZE;
}

uint32_t nn_util_mtgenerate(uint32_t* binst, uint32_t* index)
{
    uint32_t i;
    uint32_t y;
    if(*index >= MT_STATE_SIZE)
    {
        for(i = 0; i < MT_STATE_SIZE - 397; i++)
        {
            y = (binst[i] & 0x80000000) | (binst[i + 1] & 0x7fffffff);
            binst[i] = binst[i + 397] ^ (y >> 1) ^ ((y & 1) * 0x9908b0df);
        }
        for(; i < MT_STATE_SIZE - 1; i++)
        {
            y = (binst[i] & 0x80000000) | (binst[i + 1] & 0x7fffffff);
            binst[i] = binst[i + (397 - MT_STATE_SIZE)] ^ (y >> 1) ^ ((y & 1) * 0x9908b0df);
        }
        y = (binst[MT_STATE_SIZE - 1] & 0x80000000) | (binst[0] & 0x7fffffff);
        binst[MT_STATE_SIZE - 1] = binst[396] ^ (y >> 1) ^ ((y & 1) * 0x9908b0df);
        *index = 0;
    }
    y = binst[*index];
    *index = *index + 1;
    y = y ^ (y >> 11);
    y = y ^ ((y << 7) & 0x9d2c5680);
    y = y ^ ((y << 15) & 0xefc60000);
    y = y ^ (y >> 18);
    return y;
}

double nn_util_mtrand(double lowerlimit, double upperlimit)
{
    double randnum;
    uint32_t randval;
    struct timeval tv;
    static uint32_t mtstate[MT_STATE_SIZE];
    static uint32_t mtindex = MT_STATE_SIZE + 1;
    if(mtindex >= MT_STATE_SIZE)
    {
        osfn_gettimeofday(&tv, NULL);
        nn_util_mtseed((uint32_t)(1000000 * tv.tv_sec + tv.tv_usec), mtstate, &mtindex);
    }
    randval = nn_util_mtgenerate(mtstate, &mtindex);
    randnum = lowerlimit + ((double)randval / UINT32_MAX) * (upperlimit - lowerlimit);
    return randnum;
}

NNValue nn_nativefn_rand(NNState* state, NNArguments* args)
{
    int tmp;
    int lowerlimit;
    int upperlimit;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNTRANGE(&check, 0, 2);
    lowerlimit = 0;
    upperlimit = 1;
    if(args->count > 0)
    {
        NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isnumber);
        lowerlimit = nn_value_asnumber(args->args[0]);
    }
    if(args->count == 2)
    {
        NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isnumber);
        upperlimit = nn_value_asnumber(args->args[1]);
    }
    if(lowerlimit > upperlimit)
    {
        tmp = upperlimit;
        upperlimit = lowerlimit;
        lowerlimit = tmp;
    }
    return nn_value_makenumber(nn_util_mtrand(lowerlimit, upperlimit));
}

NNValue nn_nativefn_typeof(NNState* state, NNArguments* args)
{
    const char* result;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    result = nn_value_typename(args->args[0]);
    return nn_value_fromobject(nn_string_copycstr(state, result));
}

NNValue nn_nativefn_eval(NNState* state, NNArguments* args)
{
    NNValue result;
    NNObjString* os;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    os = nn_value_asstring(args->args[0]);
    /*fprintf(stderr, "eval:src=%s\n", os->sbuf->data);*/
    result = nn_state_evalsource(state, os->sbuf->data);
    return result;
}

/*
NNValue nn_nativefn_loadfile(NNState* state, NNArguments* args)
{
    NNValue result;
    NNObjString* os;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 1);
    os = nn_value_asstring(args->args[0]);
    fprintf(stderr, "eval:src=%s\n", os->sbuf->data);
    result = nn_state_evalsource(state, os->sbuf->data);
    return result;
}
*/

NNValue nn_nativefn_instanceof(NNState* state, NNArguments* args)
{
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKCOUNT(&check, 2);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isinstance);
    NEON_ARGS_CHECKTYPE(&check, 1, nn_value_isclass);
    return nn_value_makebool(nn_util_isinstanceof(nn_value_asinstance(args->args[0])->klass, nn_value_asclass(args->args[1])));
}


void nn_strformat_init(NNState* state, NNFormatInfo* nfi, NNPrinter* writer, const char* fmtstr, size_t fmtlen)
{
    nfi->pvm = state;
    nfi->fmtstr = fmtstr;
    nfi->fmtlen = fmtlen;
    nfi->writer = writer;
}

void nn_strformat_destroy(NNFormatInfo* nfi)
{
    (void)nfi;
}

bool nn_strformat_format(NNFormatInfo* nfi, int argc, int argbegin, NNValue* argv)
{
    int ch;
    int ival;
    int nextch;
    bool failed;
    size_t i;
    size_t argpos;
    NNValue cval;
    i = 0;
    argpos = argbegin;
    failed = false;
    while(i < nfi->fmtlen)
    {
        ch = nfi->fmtstr[i];
        nextch = -1;
        if((i + 1) < nfi->fmtlen)
        {
            nextch = nfi->fmtstr[i+1];
        }
        i++;
        if(ch == '%')
        {
            if(nextch == '%')
            {
                nn_printer_writechar(nfi->writer, '%');
            }
            else
            {
                i++;
                if((int)argpos > argc)
                {
                    failed = true;
                    cval = nn_value_makeempty();
                }
                else
                {
                    cval = argv[argpos];
                }
                argpos++;
                switch(nextch)
                {
                    case 'q':
                    case 'p':
                        {
                            nn_printer_printvalue(nfi->writer, cval, true, true);
                        }
                        break;
                    case 'c':
                        {
                            ival = (int)nn_value_asnumber(cval);
                            nn_printer_writefmt(nfi->writer, "%c", ival);
                        }
                        break;
                    /* TODO: implement actual field formatting */
                    case 's':
                    case 'd':
                    case 'i':
                    case 'g':
                        {
                            nn_printer_printvalue(nfi->writer, cval, false, true);
                        }
                        break;
                    default:
                        {
                            nn_exceptions_throw(nfi->pvm, "unknown/invalid format flag '%%c'", nextch);
                        }
                        break;
                }
            }
        }
        else
        {
            nn_printer_writechar(nfi->writer, ch);
        }
    }
    return failed;
}

NNValue nn_nativefn_sprintf(NNState* state, NNArguments* args)
{
    NNFormatInfo nfi;
    NNPrinter pr;
    NNObjString* res;
    NNObjString* ofmt;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKMINARG(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    ofmt = nn_value_asstring(args->args[0]);
    nn_printer_makestackstring(state, &pr);
    nn_strformat_init(state, &nfi, &pr, nn_string_getcstr(ofmt), nn_string_getlength(ofmt));
    if(!nn_strformat_format(&nfi, args->count, 1, args->args))
    {
        return nn_value_makenull();
    }
    res = nn_printer_takestring(&pr);
    return nn_value_fromobject(res);
}

NNValue nn_nativefn_printf(NNState* state, NNArguments* args)
{
    NNFormatInfo nfi;
    NNObjString* ofmt;
    NNArgCheck check;
    nn_argcheck_init(state, &check, args);
    NEON_ARGS_CHECKMINARG(&check, 1);
    NEON_ARGS_CHECKTYPE(&check, 0, nn_value_isstring);
    ofmt = nn_value_asstring(args->args[0]);
    nn_strformat_init(state, &nfi, state->stdoutprinter, nn_string_getcstr(ofmt), nn_string_getlength(ofmt));
    if(!nn_strformat_format(&nfi, args->count, 1, args->args))
    {
    }
    return nn_value_makenull();
}

NNValue nn_nativefn_print(NNState* state, NNArguments* args)
{
    size_t i;
    for(i = 0; i < args->count; i++)
    {
        nn_printer_printvalue(state->stdoutprinter, args->args[i], false, true);
    }
    if(state->isrepl)
    {
        nn_printer_writestring(state->stdoutprinter, "\n");
    }
    return nn_value_makeempty();
}

NNValue nn_nativefn_println(NNState* state, NNArguments* args)
{
    NNValue v;
    v = nn_nativefn_print(state, args);
    nn_printer_writestring(state->stdoutprinter, "\n");
    return v;
}

void nn_state_initbuiltinfunctions(NNState* state)
{
    nn_state_defnativefunction(state, "chr", nn_nativefn_chr);
    nn_state_defnativefunction(state, "id", nn_nativefn_id);
    nn_state_defnativefunction(state, "int", nn_nativefn_int);
    nn_state_defnativefunction(state, "instanceof", nn_nativefn_instanceof);
    nn_state_defnativefunction(state, "microtime", nn_nativefn_microtime);
    nn_state_defnativefunction(state, "ord", nn_nativefn_ord);
    nn_state_defnativefunction(state, "sprintf", nn_nativefn_sprintf);
    nn_state_defnativefunction(state, "printf", nn_nativefn_printf);
    nn_state_defnativefunction(state, "print", nn_nativefn_print);
    nn_state_defnativefunction(state, "println", nn_nativefn_println);
    nn_state_defnativefunction(state, "rand", nn_nativefn_rand);
    nn_state_defnativefunction(state, "time", nn_nativefn_time);
    nn_state_defnativefunction(state, "eval", nn_nativefn_eval);    
}

void nn_state_vwarn(NNState* state, const char* fmt, va_list va)
{
    if(state->conf.enablewarnings)
    {
        fprintf(stderr, "WARNING: ");
        vfprintf(stderr, fmt, va);
        fprintf(stderr, "\n");
    }
}

void nn_state_warn(NNState* state, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    nn_state_vwarn(state, fmt, va);
    va_end(va);
}

NNValue nn_exceptions_getstacktrace(NNState* state)
{
    int line;
    size_t i;
    size_t instruction;
    const char* fnname;
    const char* physfile;
    NNCallFrame* frame;
    NNObjFuncScript* function;
    NNObjString* os;
    NNObjArray* oa;
    NNPrinter pr;
    oa = nn_object_makearray(state);
    {
        for(i = 0; i < state->vmstate.framecount; i++)
        {
            nn_printer_makestackstring(state, &pr);
            frame = &state->vmstate.framevalues[i];
            function = frame->closure->scriptfunc;
            /* -1 because the IP is sitting on the next instruction to be executed */
            instruction = frame->inscode - function->blob.instrucs - 1;
            line = function->blob.instrucs[instruction].srcline;
            physfile = "(unknown)";
            if(function->module->physicalpath != NULL)
            {
                if(function->module->physicalpath->sbuf != NULL)
                {
                    physfile = function->module->physicalpath->sbuf->data;
                }
            }
            fnname = "<script>";
            if(function->name != NULL)
            {
                fnname = function->name->sbuf->data;
            }
            nn_printer_writefmt(&pr, "from %s() in %s:%d", fnname, physfile, line);
            os = nn_printer_takestring(&pr);
            nn_array_push(oa, nn_value_fromobject(os));
            if((i > 15) && (state->conf.showfullstack == false))
            {
                nn_printer_makestackstring(state, &pr);
                nn_printer_writefmt(&pr, "(only upper 15 entries shown)");
                os = nn_printer_takestring(&pr);
                nn_array_push(oa, nn_value_fromobject(os));
                break;
            }
        }
        return nn_value_fromobject(oa);
    }
    return nn_value_fromobject(nn_string_copylen(state, "", 0));
}

bool nn_exceptions_propagate(NNState* state)
{
    int i;
    int cnt;
    int srcline;
/*
{
    NEON_COLOR_RESET,
    NEON_COLOR_RED,
    NEON_COLOR_GREEN,
    NEON_COLOR_YELLOW,
    NEON_COLOR_BLUE,
    NEON_COLOR_MAGENTA,
    NEON_COLOR_CYAN
}
*/
    const char* colred;
    const char* colreset;
    const char* colyellow;
    const char* srcfile;
    NNValue stackitm;
    NNObjArray* oa;
    NNObjFuncScript* function;
    NNExceptionFrame* handler;
    NNObjString* emsg;
    NNObjInstance* exception;
    NNProperty* field;
    exception = nn_value_asinstance(nn_vm_stackpeek(state, 0));
    while(state->vmstate.framecount > 0)
    {
        state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
        for(i = state->vmstate.currentframe->handlercount; i > 0; i--)
        {
            handler = &state->vmstate.currentframe->handlers[i - 1];
            function = state->vmstate.currentframe->closure->scriptfunc;
            if(handler->address != 0 && nn_util_isinstanceof(exception->klass, handler->klass))
            {
                state->vmstate.currentframe->inscode = &function->blob.instrucs[handler->address];
                return true;
            }
            else if(handler->finallyaddress != 0)
            {
                /* continue propagating once the 'finally' block completes */
                nn_vm_stackpush(state, nn_value_makebool(true));
                state->vmstate.currentframe->inscode = &function->blob.instrucs[handler->finallyaddress];
                return true;
            }
        }
        state->vmstate.framecount--;
    }
    colred = nn_util_color(NEON_COLOR_RED);
    colreset = nn_util_color(NEON_COLOR_RESET);
    colyellow = nn_util_color(NEON_COLOR_YELLOW);
    /* at this point, the exception is unhandled; so, print it out. */
    fprintf(stderr, "%sunhandled %s%s", colred, exception->klass->name->sbuf->data, colreset);
    srcfile = "none";
    srcline = 0;
    field = nn_table_getfieldbycstr(exception->properties, "srcline");
    if(field != NULL)
    {
        srcline = nn_value_asnumber(field->value);
    }
    field = nn_table_getfieldbycstr(exception->properties, "srcfile");
    if(field != NULL)
    {
        srcfile = nn_value_asstring(field->value)->sbuf->data;
    }
    fprintf(stderr, " [from native %s%s:%d%s]", colyellow, srcfile, srcline, colreset);
    
    field = nn_table_getfieldbycstr(exception->properties, "message");
    if(field != NULL)
    {
        emsg = nn_value_tostring(state, field->value);
        if(emsg->sbuf->length > 0)
        {
            fprintf(stderr, ": %s", emsg->sbuf->data);
        }
        else
        {
            fprintf(stderr, ":");
        }
        fprintf(stderr, "\n");
    }
    else
    {
        fprintf(stderr, "\n");
    }
    field = nn_table_getfieldbycstr(exception->properties, "stacktrace");
    if(field != NULL)
    {
        fprintf(stderr, "  stacktrace:\n");
        oa = nn_value_asarray(field->value);
        cnt = oa->varray->listcount;
        i = cnt-1;
        if(cnt > 0)
        {
            while(true)
            {
                stackitm = oa->varray->listitems[i];
                nn_printer_writefmt(state->debugwriter, "  ");
                nn_printer_printvalue(state->debugwriter, stackitm, false, true);
                nn_printer_writefmt(state->debugwriter, "\n");
                if(i == 0)
                {
                    break;
                }
                i--;
            }
        }
    }
    return false;
}

bool nn_exceptions_pushhandler(NNState* state, NNObjClass* type, int address, int finallyaddress)
{
    NNCallFrame* frame;
    frame = &state->vmstate.framevalues[state->vmstate.framecount - 1];
    if(frame->handlercount == NEON_CFG_MAXEXCEPTHANDLERS)
    {
        nn_vm_raisefatalerror(state, "too many nested exception handlers in one function");
        return false;
    }
    frame->handlers[frame->handlercount].address = address;
    frame->handlers[frame->handlercount].finallyaddress = finallyaddress;
    frame->handlers[frame->handlercount].klass = type;
    frame->handlercount++;
    return true;
}


bool nn_exceptions_vthrowactual(NNState* state, NNObjClass* klass, const char* srcfile, int srcline, const char* format, va_list va)
{
    bool b;
    b = nn_exceptions_vthrowwithclass(state, klass, srcfile, srcline, format, va);
    return b;
}

bool nn_exceptions_throwactual(NNState* state, NNObjClass* klass, const char* srcfile, int srcline, const char* format, ...)
{
    bool b;
    va_list va;
    va_start(va, format);
    b = nn_exceptions_vthrowactual(state, klass, srcfile, srcline, format, va);
    va_end(va);
    return b;
}

bool nn_exceptions_throwwithclass(NNState* state, NNObjClass* klass, const char* srcfile, int srcline, const char* format, ...)
{
    bool b;
    va_list args;
    va_start(args, format);
    b = nn_exceptions_vthrowwithclass(state, klass, srcfile, srcline, format, args);
    va_end(args);
    return b;
}

bool nn_exceptions_vthrowwithclass(NNState* state, NNObjClass* exklass, const char* srcfile, int srcline, const char* format, va_list args)
{
    int length;
    int needed;
    char* message;
    va_list vcpy;
    NNValue stacktrace;
    NNObjInstance* instance;
    va_copy(vcpy, args);
    /* TODO: used to be vasprintf. need to check how much to actually allocate! */
    needed = vsnprintf(NULL, 0, format, vcpy);
    needed += 1;
    va_end(vcpy);
    message = (char*)nn_util_memmalloc(state, needed+1);
    length = vsnprintf(message, needed, format, args);
    instance = nn_exceptions_makeinstance(state, exklass, srcfile, srcline, nn_string_takelen(state, message, length));
    nn_vm_stackpush(state, nn_value_fromobject(instance));
    stacktrace = nn_exceptions_getstacktrace(state);
    nn_vm_stackpush(state, stacktrace);
    nn_instance_defproperty(instance, "stacktrace", stacktrace);
    nn_vm_stackpop(state);
    return nn_exceptions_propagate(state);
}

NEON_FORCEINLINE NNInstruction nn_util_makeinst(bool isop, uint8_t code, int srcline)
{
    NNInstruction inst;
    inst.isop = isop;
    inst.code = code;
    inst.srcline = srcline;
    return inst;
}

NNObjClass* nn_exceptions_makeclass(NNState* state, NNObjModule* module, const char* cstrname)
{
    int messageconst;
    NNObjClass* klass;
    NNObjString* classname;
    NNObjFuncScript* function;
    NNObjFuncClosure* closure;
    classname = nn_string_copycstr(state, cstrname);
    nn_vm_stackpush(state, nn_value_fromobject(classname));
    klass = nn_object_makeclass(state, classname);
    nn_vm_stackpop(state);
    nn_vm_stackpush(state, nn_value_fromobject(klass));
    function = nn_object_makefuncscript(state, module, NEON_FUNCTYPE_METHOD);
    function->arity = 1;
    function->isvariadic = false;
    nn_vm_stackpush(state, nn_value_fromobject(function));
    {
        /* g_loc 0 */
        nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_LOCALGET, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(false, (0 >> 8) & 0xff, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(false, 0 & 0xff, 0));
    }
    {
        /* g_loc 1 */
        nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_LOCALGET, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(false, (1 >> 8) & 0xff, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(false, 1 & 0xff, 0));
    }
    {
        messageconst = nn_blob_pushconst(state, &function->blob, nn_value_fromobject(nn_string_copycstr(state, "message")));
        /* s_prop 0 */
        nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_PROPERTYSET, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(false, (messageconst >> 8) & 0xff, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(false, messageconst & 0xff, 0));
    }
    {
        /* pop */
        nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_POPONE, 0));
        nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_POPONE, 0));
    }
    {
        /* g_loc 0 */
        /*
        //  nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_LOCALGET, 0));
        //  nn_blob_push(state, &function->blob, nn_util_makeinst(false, (0 >> 8) & 0xff, 0));
        //  nn_blob_push(state, &function->blob, nn_util_makeinst(false, 0 & 0xff, 0));
        */
    }
    {
        /* ret */
        nn_blob_push(state, &function->blob, nn_util_makeinst(true, NEON_OP_RETURN, 0));
    }
    closure = nn_object_makefuncclosure(state, function);
    nn_vm_stackpop(state);
    /* set class constructor */
    nn_vm_stackpush(state, nn_value_fromobject(closure));
    nn_table_set(klass->methods, nn_value_fromobject(classname), nn_value_fromobject(closure));
    klass->constructor = nn_value_fromobject(closure);
    /* set class properties */
    nn_class_defproperty(klass, "message", nn_value_makenull());
    nn_class_defproperty(klass, "stacktrace", nn_value_makenull());
    nn_table_set(state->globals, nn_value_fromobject(classname), nn_value_fromobject(klass));
    /* for class */
    nn_vm_stackpop(state);
    nn_vm_stackpop(state);
    /* assert error name */
    /* nn_vm_stackpop(state); */
    return klass;
}

NNObjInstance* nn_exceptions_makeinstance(NNState* state, NNObjClass* exklass, const char* srcfile, int srcline, NNObjString* message)
{
    NNObjInstance* instance;
    NNObjString* osfile;
    instance = nn_object_makeinstance(state, exklass);
    osfile = nn_string_copycstr(state, srcfile);
    nn_vm_stackpush(state, nn_value_fromobject(instance));
    nn_instance_defproperty(instance, "message", nn_value_fromobject(message));
    nn_instance_defproperty(instance, "srcfile", nn_value_fromobject(osfile));
    nn_instance_defproperty(instance, "srcline", nn_value_makenumber(srcline));
    nn_vm_stackpop(state);
    return instance;
}

void nn_vm_raisefatalerror(NNState* state, const char* format, ...)
{
    int i;
    int line;
    size_t instruction;
    va_list args;
    NNCallFrame* frame;
    NNObjFuncScript* function;
    /* flush out anything on stdout first */
    fflush(stdout);
    frame = &state->vmstate.framevalues[state->vmstate.framecount - 1];
    function = frame->closure->scriptfunc;
    instruction = frame->inscode - function->blob.instrucs - 1;
    line = function->blob.instrucs[instruction].srcline;
    fprintf(stderr, "RuntimeError: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, " -> %s:%d ", function->module->physicalpath->sbuf->data, line);
    fputs("\n", stderr);
    if(state->vmstate.framecount > 1)
    {
        fprintf(stderr, "stacktrace:\n");
        for(i = state->vmstate.framecount - 1; i >= 0; i--)
        {
            frame = &state->vmstate.framevalues[i];
            function = frame->closure->scriptfunc;
            /* -1 because the IP is sitting on the next instruction to be executed */
            instruction = frame->inscode - function->blob.instrucs - 1;
            fprintf(stderr, "    %s:%d -> ", function->module->physicalpath->sbuf->data, function->blob.instrucs[instruction].srcline);
            if(function->name == NULL)
            {
                fprintf(stderr, "<script>");
            }
            else
            {
                fprintf(stderr, "%s()", function->name->sbuf->data);
            }
            fprintf(stderr, "\n");
        }
    }
    nn_state_resetvmstate(state);
}

void nn_state_defglobalvalue(NNState* state, const char* name, NNValue val)
{
    NNObjString* oname;
    oname = nn_string_copycstr(state, name);
    nn_vm_stackpush(state, nn_value_fromobject(oname));
    nn_vm_stackpush(state, val);
    nn_table_set(state->globals, state->vmstate.stackvalues[0], state->vmstate.stackvalues[1]);
    nn_vm_stackpopn(state, 2);
}

void nn_state_defnativefunctionptr(NNState* state, const char* name, NNNativeFN fptr, void* uptr)
{
    NNObjFuncNative* func;
    func = nn_object_makefuncnative(state, fptr, name, uptr);
    return nn_state_defglobalvalue(state, name, nn_value_fromobject(func));
}

void nn_state_defnativefunction(NNState* state, const char* name, NNNativeFN fptr)
{
    return nn_state_defnativefunctionptr(state, name, fptr, NULL);
}

NNObjClass* nn_util_makeclass(NNState* state, const char* name, NNObjClass* parent)
{
    NNObjClass* cl;
    NNObjString* os;
    os = nn_string_copycstr(state, name);
    cl = nn_object_makeclass(state, os);
    cl->superclass = parent;
    nn_table_set(state->globals, nn_value_fromobject(os), nn_value_fromobject(cl));
    return cl;
}

void nn_vm_initvmstate(NNState* state)
{
    state->vmstate.linkedobjects = NULL;
    state->vmstate.currentframe = NULL;
    {
        state->vmstate.stackcapacity = NEON_CFG_INITSTACKCOUNT;
        state->vmstate.stackvalues = (NNValue*)nn_util_memmalloc(state, NEON_CFG_INITSTACKCOUNT * sizeof(NNValue));
        if(state->vmstate.stackvalues == NULL)
        {
            fprintf(stderr, "error: failed to allocate stackvalues!\n");
            abort();
        }
        memset(state->vmstate.stackvalues, 0, NEON_CFG_INITSTACKCOUNT * sizeof(NNValue));
    }
    {
        state->vmstate.framecapacity = NEON_CFG_INITFRAMECOUNT;
        state->vmstate.framevalues = (NNCallFrame*)nn_util_memmalloc(state, NEON_CFG_INITFRAMECOUNT * sizeof(NNCallFrame));
        if(state->vmstate.framevalues == NULL)
        {
            fprintf(stderr, "error: failed to allocate framevalues!\n");
            abort();
        }
        memset(state->vmstate.framevalues, 0, NEON_CFG_INITFRAMECOUNT * sizeof(NNCallFrame));
    }
}

bool nn_vm_checkmayberesize(NNState* state)
{
    if((state->vmstate.stackidx+1) >= state->vmstate.stackcapacity)
    {
        if(!nn_vm_resizestack(state, state->vmstate.stackidx + 1))
        {
            return nn_exceptions_throw(state, "failed to resize stack due to overflow");
        }
        return true;
    }
    if(state->vmstate.framecount >= state->vmstate.framecapacity)
    {
        if(!nn_vm_resizeframes(state, state->vmstate.framecapacity + 1))
        {
            return nn_exceptions_throw(state, "failed to resize frames due to overflow");
        }
        return true;
    }
    return false;
}

/*
* grows vmstate.(stack|frame)values, respectively.
* currently it works fine with mob.js (man-or-boy test), although
* there are still some invalid reads regarding the closure;
* almost definitely because the pointer address changes.
*
* currently, the implementation really does just increase the
* memory block available:
* i.e., [NNValue x 32] -> [NNValue x <newsize>], without
* copying anything beyond primitive values.
*/
bool nn_vm_resizestack(NNState* state, size_t needed)
{
    size_t oldsz;
    size_t newsz;
    size_t allocsz;
    size_t nforvals;
    NNValue* oldbuf;
    NNValue* newbuf;
    nforvals = (needed * 2);
    oldsz = state->vmstate.stackcapacity;
    newsz = oldsz + nforvals;
    allocsz = ((newsz + 1) * sizeof(NNValue));
    fprintf(stderr, "*** resizing stack: needed %ld, from %ld to %ld, allocating %ld ***\n", nforvals, oldsz, newsz, allocsz);
    oldbuf = state->vmstate.stackvalues;
    newbuf = (NNValue*)nn_util_memrealloc(state, oldbuf, allocsz);
    if(newbuf == NULL)
    {
        fprintf(stderr, "internal error: failed to resize stackvalues!\n");
        abort();
    }
    state->vmstate.stackvalues = (NNValue*)newbuf;
    state->vmstate.stackcapacity = newsz;
    return true;
}

bool nn_vm_resizeframes(NNState* state, size_t needed)
{
    /* return false; */
    size_t i;
    size_t oldsz;
    size_t newsz;
    size_t allocsz;
    int oldhandlercnt;
    NNInstruction* oldip;
    NNObjFuncClosure* oldclosure;
    NNCallFrame* oldbuf;
    NNCallFrame* newbuf;
    (void)i;
    fprintf(stderr, "*** resizing frames ***\n");
    oldclosure = state->vmstate.currentframe->closure;
    oldip = state->vmstate.currentframe->inscode;
    oldhandlercnt = state->vmstate.currentframe->handlercount;
    oldsz = state->vmstate.framecapacity;
    newsz = oldsz + needed;
    allocsz = ((newsz + 1) * sizeof(NNCallFrame));
    #if 1
        oldbuf = state->vmstate.framevalues;
        newbuf = (NNCallFrame*)nn_util_memrealloc(state, oldbuf, allocsz);
        if(newbuf == NULL)
        {
            fprintf(stderr, "internal error: failed to resize framevalues!\n");
            abort();
        }
    #endif
    state->vmstate.framevalues = (NNCallFrame*)newbuf;
    state->vmstate.framecapacity = newsz;
    /*
    * this bit is crucial: realloc changes pointer addresses, and to keep the
    * current frame, re-read it from the new address.
    */
    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
    state->vmstate.currentframe->handlercount = oldhandlercnt;
    state->vmstate.currentframe->inscode = oldip;
    state->vmstate.currentframe->closure = oldclosure;
    return true;
}

void nn_state_resetvmstate(NNState* state)
{
    state->vmstate.framecount = 0;
    state->vmstate.stackidx = 0;
    state->vmstate.openupvalues = NULL;
}

bool nn_state_addsearchpathobj(NNState* state, NNObjString* os)
{
    nn_vallist_push(state->importpath, nn_value_fromobject(os));
    return true;
}

bool nn_state_addsearchpath(NNState* state, const char* path)
{
    return nn_state_addsearchpathobj(state, nn_string_copycstr(state, path));
}

NNState* nn_state_make()
{
    return nn_state_makewithuserptr(NULL);
}

NNState* nn_state_makewithuserptr(void* userptr)
{
    enum{ kMaxBuf = 1024 };
    static const char* defaultsearchpaths[] =
    {
        "mods",
        "mods/@/index" NEON_CFG_FILEEXT,
        ".",
        NULL
    };
    size_t i;
    char* pathp;
    char pathbuf[kMaxBuf];
    NNState* state;
    state = (NNState*)nn_util_rawmalloc(userptr, sizeof(NNState));
    if(state == NULL)
    {
        return NULL;
    }
    memset(state, 0, sizeof(NNState));
    state->memuserptr = userptr;
    state->exceptions.stdexception = NULL;
    state->rootphysfile = NULL;
    state->cliargv = NULL;
    state->isrepl = false;
    state->markvalue = true;
    nn_vm_initvmstate(state);
    nn_state_resetvmstate(state);
    {
        state->conf.enablestrictmode = false;
        state->conf.shoulddumpstack = false;
        state->conf.enablewarnings = false;
        state->conf.dumpbytecode = false;
        state->conf.exitafterbytecode = false;
        state->conf.showfullstack = false;
        state->conf.enableapidebug = false;
        state->conf.enableastdebug = false;
    }
    {
        state->gcstate.bytesallocated = 0;
        /* default is 1mb. Can be modified via the -g flag. */
        state->gcstate.nextgc = NEON_CFG_DEFAULTGCSTART;
        state->gcstate.graycount = 0;
        state->gcstate.graycapacity = 0;
        state->gcstate.graystack = NULL;
    }
    {
        state->stdoutprinter = nn_printer_makeio(state, stdout, false);
        state->stdoutprinter->shouldflush = true;
        state->stderrprinter = nn_printer_makeio(state, stderr, false);
        state->debugwriter = nn_printer_makeio(state, stderr, false);
        state->debugwriter->shortenvalues = true;
        state->debugwriter->maxvallength = 15;
    }
    {
        state->modules = nn_table_make(state);
        state->strings = nn_table_make(state);
        state->globals = nn_table_make(state);
    }
    {
        state->topmodule = nn_module_make(state, "", "<state>", false);
        state->constructorname = nn_string_copycstr(state, "constructor");
    }
    {
        state->importpath = nn_vallist_make(state);
        for(i=0; defaultsearchpaths[i]!=NULL; i++)
        {
            nn_state_addsearchpath(state, defaultsearchpaths[i]);
        }
    }
    {
        state->classprimobject = nn_util_makeclass(state, "Object", NULL);
        state->classprimprocess = nn_util_makeclass(state, "Process", state->classprimobject);
        state->classprimnumber = nn_util_makeclass(state, "Number", state->classprimobject);
        state->classprimstring = nn_util_makeclass(state, "String", state->classprimobject);
        state->classprimarray = nn_util_makeclass(state, "Array", state->classprimobject);
        state->classprimdict = nn_util_makeclass(state, "Dict", state->classprimobject);
        state->classprimfile = nn_util_makeclass(state, "File", state->classprimobject);
        state->classprimrange = nn_util_makeclass(state, "Range", state->classprimobject);
        state->classprimmath = nn_util_makeclass(state, "Math", state->classprimobject);
    }
    {
        state->envdict = nn_object_makedict(state);
    }
    {
        if(state->exceptions.stdexception == NULL)
        {
            state->exceptions.stdexception = nn_exceptions_makeclass(state, NULL, "Exception");
        }
        state->exceptions.asserterror = nn_exceptions_makeclass(state, NULL, "AssertError");
        state->exceptions.syntaxerror = nn_exceptions_makeclass(state, NULL, "SyntaxError");
        state->exceptions.ioerror = nn_exceptions_makeclass(state, NULL, "IOError");
        state->exceptions.oserror = nn_exceptions_makeclass(state, NULL, "OSError");
        state->exceptions.argumenterror = nn_exceptions_makeclass(state, NULL, "ArgumentError");
    }
    {
        nn_state_initbuiltinfunctions(state);
        nn_state_initbuiltinmethods(state);
    }
    {
        pathp = osfn_getcwd(pathbuf, kMaxBuf);
        state->clidirectory = nn_string_copycstr(state, pathp);
    }
    {
        {
            state->filestdout = nn_object_makefile(state, stdout, true, "<stdout>", "wb");
            nn_state_defglobalvalue(state, "STDOUT", nn_value_fromobject(state->filestdout));
        }
        {
            state->filestderr = nn_object_makefile(state, stderr, true, "<stderr>", "wb");
            nn_state_defglobalvalue(state, "STDERR", nn_value_fromobject(state->filestderr));
        }
        {
            state->filestdin = nn_object_makefile(state, stdin, true, "<stdin>", "rb");
            nn_state_defglobalvalue(state, "STDIN", nn_value_fromobject(state->filestdin));
        }
    }
    return state;
}

#if 0
    #define destrdebug(...) \
        { \
            fprintf(stderr, "in nn_state_destroy: "); \
            fprintf(stderr, __VA_ARGS__); \
            fprintf(stderr, "\n"); \
        }
#else
    #define destrdebug(...)
#endif
void nn_state_destroy(NNState* state)
{
    destrdebug("destroying importpath...");
    nn_vallist_destroy(state->importpath);
    destrdebug("destroying linked objects...");
    nn_gcmem_destroylinkedobjects(state);
    /* since object in module can exist in globals, it must come before */
    destrdebug("destroying module table...");
    nn_table_destroy(state->modules);
    destrdebug("destroying globals table...");
    nn_table_destroy(state->globals);
    destrdebug("destroying strings table...");
    nn_table_destroy(state->strings);
    destrdebug("destroying stdoutprinter...");
    nn_printer_destroy(state->stdoutprinter);
    destrdebug("destroying stderrprinter...");
    nn_printer_destroy(state->stderrprinter);
    destrdebug("destroying debugwriter...");
    nn_printer_destroy(state->debugwriter);
    destrdebug("destroying framevalues...");
    nn_util_memfree(state, state->vmstate.framevalues);
    destrdebug("destroying stackvalues...");
    nn_util_memfree(state, state->vmstate.stackvalues);
    destrdebug("destroying state...");
    nn_util_memfree(state, state);
    destrdebug("done destroying!");
}

bool nn_util_methodisprivate(NNObjString* name)
{
    return name->sbuf->length > 0 && name->sbuf->data[0] == '_';
}

bool nn_vm_callclosure(NNState* state, NNObjFuncClosure* closure, NNValue thisval, int argcount)
{
    int i;
    int startva;
    NNCallFrame* frame;
    NNObjArray* argslist;
    (void)thisval;
    NEON_APIDEBUG(state, "thisval.type=%s, argcount=%d", nn_value_typename(thisval), argcount);
    /* fill empty parameters if not variadic */
    for(; !closure->scriptfunc->isvariadic && argcount < closure->scriptfunc->arity; argcount++)
    {
        nn_vm_stackpush(state, nn_value_makenull());
    }
    /* handle variadic arguments... */
    if(closure->scriptfunc->isvariadic && argcount >= closure->scriptfunc->arity - 1)
    {
        startva = argcount - closure->scriptfunc->arity;
        argslist = nn_object_makearray(state);
        nn_vm_stackpush(state, nn_value_fromobject(argslist));
        for(i = startva; i >= 0; i--)
        {
            nn_vallist_push(argslist->varray, nn_vm_stackpeek(state, i + 1));
        }
        argcount -= startva;
        /* +1 for the gc protection push above */
        nn_vm_stackpopn(state, startva + 2);
        nn_vm_stackpush(state, nn_value_fromobject(argslist));
    }
    if(argcount != closure->scriptfunc->arity)
    {
        nn_vm_stackpopn(state, argcount);
        if(closure->scriptfunc->isvariadic)
        {
            return nn_exceptions_throw(state, "expected at least %d arguments but got %d", closure->scriptfunc->arity - 1, argcount);
        }
        else
        {
            return nn_exceptions_throw(state, "expected %d arguments but got %d", closure->scriptfunc->arity, argcount);
        }
    }
    if(nn_vm_checkmayberesize(state))
    {
        /* nn_vm_stackpopn(state, argcount); */
    }
    frame = &state->vmstate.framevalues[state->vmstate.framecount++];
    frame->closure = closure;
    frame->inscode = closure->scriptfunc->blob.instrucs;
    frame->stackslotpos = state->vmstate.stackidx + (-argcount - 1);
    return true;
}

bool nn_vm_callnative(NNState* state, NNObjFuncNative* native, NNValue thisval, int argcount)
{
    size_t spos;
    NNValue r;
    NNValue* vargs;
    NNArguments fnargs;
    NEON_APIDEBUG(state, "thisval.type=%s, argcount=%d", nn_value_typename(thisval), argcount);
    spos = state->vmstate.stackidx + (-argcount);
    vargs = &state->vmstate.stackvalues[spos];
    fnargs.count = argcount;
    fnargs.args = vargs;
    fnargs.thisval = thisval;
    fnargs.userptr = native->userptr;
    fnargs.name = native->name;
    r = native->natfunc(state, &fnargs);
    {
        state->vmstate.stackvalues[spos - 1] = r;
        state->vmstate.stackidx -= argcount;
    }
    nn_gcmem_clearprotect(state);
    return true;
}

bool nn_vm_callvaluewithobject(NNState* state, NNValue callable, NNValue thisval, int argcount)
{
    size_t spos;
    NEON_APIDEBUG(state, "thisval.type=%s, argcount=%d", nn_value_typename(thisval), argcount);
    if(nn_value_isobject(callable))
    {
        switch(nn_value_objtype(callable))
        {
            case NEON_OBJTYPE_FUNCBOUND:
                {
                    NNObjFuncBound* bound;
                    bound = nn_value_asfuncbound(callable);
                    spos = (state->vmstate.stackidx + (-argcount - 1));
                    state->vmstate.stackvalues[spos] = thisval;
                    return nn_vm_callclosure(state, bound->method, thisval, argcount);
                }
                break;
            case NEON_OBJTYPE_CLASS:
                {
                    NNObjClass* klass;
                    klass = nn_value_asclass(callable);
                    spos = (state->vmstate.stackidx + (-argcount - 1));
                    state->vmstate.stackvalues[spos] = thisval;
                    if(!nn_value_isempty(klass->constructor))
                    {
                        return nn_vm_callvaluewithobject(state, klass->constructor, thisval, argcount);
                    }
                    else if(klass->superclass != NULL && !nn_value_isempty(klass->superclass->constructor))
                    {
                        return nn_vm_callvaluewithobject(state, klass->superclass->constructor, thisval, argcount);
                    }
                    else if(argcount != 0)
                    {
                        return nn_exceptions_throw(state, "%s constructor expects 0 arguments, %d given", klass->name->sbuf->data, argcount);
                    }
                    return true;
                }
                break;
            case NEON_OBJTYPE_MODULE:
                {
                    NNObjModule* module;
                    NNProperty* field;
                    module = nn_value_asmodule(callable);
                    field = nn_table_getfieldbyostr(module->deftable, module->name);
                    if(field != NULL)
                    {
                        return nn_vm_callvalue(state, field->value, thisval, argcount);
                    }
                    return nn_exceptions_throw(state, "module %s does not export a default function", module->name);
                }
                break;
            case NEON_OBJTYPE_FUNCCLOSURE:
                {
                    return nn_vm_callclosure(state, nn_value_asfuncclosure(callable), thisval, argcount);
                }
                break;
            case NEON_OBJTYPE_FUNCNATIVE:
                {
                    return nn_vm_callnative(state, nn_value_asfuncnative(callable), thisval, argcount);
                }
                break;
            default:
                break;
        }
    }
    return nn_exceptions_throw(state, "object of type %s is not callable", nn_value_typename(callable));
}

bool nn_vm_callvalue(NNState* state, NNValue callable, NNValue thisval, int argcount)
{
    NNValue actualthisval;
    if(nn_value_isobject(callable))
    {
        switch(nn_value_objtype(callable))
        {
            case NEON_OBJTYPE_FUNCBOUND:
                {
                    NNObjFuncBound* bound;
                    bound = nn_value_asfuncbound(callable);
                    actualthisval = bound->receiver;
                    if(!nn_value_isempty(thisval))
                    {
                        actualthisval = thisval;
                    }
                    NEON_APIDEBUG(state, "actualthisval.type=%s, argcount=%d", nn_value_typename(actualthisval), argcount);
                    return nn_vm_callvaluewithobject(state, callable, actualthisval, argcount);
                }
                break;
            case NEON_OBJTYPE_CLASS:
                {
                    NNObjClass* klass;
                    NNObjInstance* instance;
                    klass = nn_value_asclass(callable);
                    instance = nn_object_makeinstance(state, klass);
                    actualthisval = nn_value_fromobject(instance);
                    if(!nn_value_isempty(thisval))
                    {
                        actualthisval = thisval;
                    }
                    NEON_APIDEBUG(state, "actualthisval.type=%s, argcount=%d", nn_value_typename(actualthisval), argcount);
                    return nn_vm_callvaluewithobject(state, callable, actualthisval, argcount);
                }
                break;
            default:
                {
                }
                break;
        }
    }
    NEON_APIDEBUG(state, "thisval.type=%s, argcount=%d", nn_value_typename(thisval), argcount);
    return nn_vm_callvaluewithobject(state, callable, thisval, argcount);
}

NNFuncType nn_value_getmethodtype(NNValue method)
{
    switch(nn_value_objtype(method))
    {
        case NEON_OBJTYPE_FUNCNATIVE:
            return nn_value_asfuncnative(method)->type;
        case NEON_OBJTYPE_FUNCCLOSURE:
            return nn_value_asfuncclosure(method)->scriptfunc->type;
        default:
            break;
    }
    return NEON_FUNCTYPE_FUNCTION;
}


NNObjClass* nn_value_getclassfor(NNState* state, NNValue receiver)
{
    if(nn_value_isnumber(receiver))
    {
        return state->classprimnumber;
    }
    if(nn_value_isobject(receiver))
    {
        switch(nn_value_asobject(receiver)->type)
        {
            case NEON_OBJTYPE_STRING:
                return state->classprimstring;
            case NEON_OBJTYPE_RANGE:
                return state->classprimrange;
            case NEON_OBJTYPE_ARRAY:
                return state->classprimarray;
            case NEON_OBJTYPE_DICT:
                return state->classprimdict;
            case NEON_OBJTYPE_FILE:
                return state->classprimfile;
            /*
            case NEON_OBJTYPE_FUNCBOUND:
            case NEON_OBJTYPE_FUNCCLOSURE:
            case NEON_OBJTYPE_FUNCSCRIPT:
                return state->classprimcallable;
            */
            default:
                {
                    fprintf(stderr, "getclassfor: unhandled type!\n");
                }
                break;
        }
    }
    return NULL;
}

NEON_FORCEINLINE void nn_vmbits_stackpush(NNState* state, NNValue value)
{
    nn_vm_checkmayberesize(state);
    state->vmstate.stackvalues[state->vmstate.stackidx] = value;
    state->vmstate.stackidx++;
}

NEON_FORCEINLINE void nn_vm_stackpush(NNState* state, NNValue value)
{
    nn_vmbits_stackpush(state, value);
}

NEON_FORCEINLINE NNValue nn_vmbits_stackpop(NNState* state)
{
    NNValue v;
    state->vmstate.stackidx--;
    v = state->vmstate.stackvalues[state->vmstate.stackidx];
    return v;
}

NEON_FORCEINLINE NNValue nn_vm_stackpop(NNState* state)
{
    return nn_vmbits_stackpop(state);
}

NEON_FORCEINLINE NNValue nn_vmbits_stackpopn(NNState* state, int n)
{
    NNValue v;
    state->vmstate.stackidx -= n;
    v = state->vmstate.stackvalues[state->vmstate.stackidx];
    return v;
}

NEON_FORCEINLINE NNValue nn_vm_stackpopn(NNState* state, int n)
{
    return nn_vmbits_stackpopn(state, n);
}

NEON_FORCEINLINE NNValue nn_vmbits_stackpeek(NNState* state, int distance)
{
    NNValue v;
    v = state->vmstate.stackvalues[state->vmstate.stackidx + (-1 - distance)];
    return v;
}

NEON_FORCEINLINE NNValue nn_vm_stackpeek(NNState* state, int distance)
{
    return nn_vmbits_stackpeek(state, distance);
}

#define nn_vmmac_exitvm(state) \
    { \
        (void)you_are_calling_exit_vm_outside_of_runvm; \
        return NEON_STATUS_FAILRUNTIME; \
    }        

#define nn_vmmac_tryraise(state, rtval, ...) \
    if(!nn_exceptions_throw(state, ##__VA_ARGS__)) \
    { \
        return rtval; \
    }

NEON_FORCEINLINE uint8_t nn_vmbits_readbyte(NNState* state)
{
    uint8_t r;
    r = state->vmstate.currentframe->inscode->code;
    state->vmstate.currentframe->inscode++;
    return r;
}

NEON_FORCEINLINE NNInstruction nn_vmbits_readinstruction(NNState* state)
{
    NNInstruction r;
    r = *state->vmstate.currentframe->inscode;
    state->vmstate.currentframe->inscode++;
    return r;
}

NEON_FORCEINLINE uint16_t nn_vmbits_readshort(NNState* state)
{
    uint8_t b;
    uint8_t a;
    a = state->vmstate.currentframe->inscode[0].code;
    b = state->vmstate.currentframe->inscode[1].code;
    state->vmstate.currentframe->inscode += 2;
    return (uint16_t)((a << 8) | b);
}

NEON_FORCEINLINE NNValue nn_vmbits_readconst(NNState* state)
{
    uint16_t idx;
    idx = nn_vmbits_readshort(state);
    return state->vmstate.currentframe->closure->scriptfunc->blob.constants->listitems[idx];
}

NEON_FORCEINLINE NNObjString* nn_vmbits_readstring(NNState* state)
{
    return nn_value_asstring(nn_vmbits_readconst(state));
}

NEON_FORCEINLINE bool nn_vmutil_invokemethodfromclass(NNState* state, NNObjClass* klass, NNObjString* name, int argcount)
{
    NNProperty* field;
    NEON_APIDEBUG(state, "argcount=%d", argcount);
    field = nn_table_getfieldbyostr(klass->methods, name);
    if(field != NULL)
    {
        if(nn_value_getmethodtype(field->value) == NEON_FUNCTYPE_PRIVATE)
        {
            return nn_exceptions_throw(state, "cannot call private method '%s' from instance of %s", name->sbuf->data, klass->name->sbuf->data);
        }
        return nn_vm_callvaluewithobject(state, field->value, nn_value_fromobject(klass), argcount);
    }
    return nn_exceptions_throw(state, "undefined method '%s' in %s", name->sbuf->data, klass->name->sbuf->data);
}

NEON_FORCEINLINE bool nn_vmutil_invokemethodself(NNState* state, NNObjString* name, int argcount)
{
    size_t spos;
    NNValue receiver;
    NNObjInstance* instance;
    NNProperty* field;
    NEON_APIDEBUG(state, "argcount=%d", argcount);
    receiver = nn_vmbits_stackpeek(state, argcount);
    if(nn_value_isinstance(receiver))
    {
        instance = nn_value_asinstance(receiver);
        field = nn_table_getfieldbyostr(instance->klass->methods, name);
        if(field != NULL)
        {
            return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
        }
        field = nn_table_getfieldbyostr(instance->properties, name);
        if(field != NULL)
        {
            spos = (state->vmstate.stackidx + (-argcount - 1));
            #if 0
                state->vmstate.stackvalues[spos] = field->value;
                return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
            #else
                state->vmstate.stackvalues[spos] = receiver;
                return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
            #endif
        }
    }
    else if(nn_value_isclass(receiver))
    {
        field = nn_table_getfieldbyostr(nn_value_asclass(receiver)->methods, name);
        if(field != NULL)
        {
            if(nn_value_getmethodtype(field->value) == NEON_FUNCTYPE_STATIC)
            {
                return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
            }
            return nn_exceptions_throw(state, "cannot call non-static method %s() on non instance", name->sbuf->data);
        }
    }
    return nn_exceptions_throw(state, "cannot call method '%s' on object of type '%s'", name->sbuf->data, nn_value_typename(receiver));
}

NEON_FORCEINLINE bool nn_vmutil_invokemethod(NNState* state, NNObjString* name, int argcount)
{
    size_t spos;
    NNObjType rectype;
    NNValue receiver;
    NNProperty* field;
    NNObjClass* klass;
    receiver = nn_vmbits_stackpeek(state, argcount);
    NEON_APIDEBUG(state, "receiver.type=%s, argcount=%d", nn_value_typename(receiver), argcount);
    if(nn_value_isobject(receiver))
    {
        rectype = nn_value_asobject(receiver)->type;
        switch(rectype)
        {
            case NEON_OBJTYPE_MODULE:
                {
                    NNObjModule* module;
                    NEON_APIDEBUG(state, "receiver is a module");
                    module = nn_value_asmodule(receiver);
                    field = nn_table_getfieldbyostr(module->deftable, name);
                    if(field != NULL)
                    {
                        if(nn_util_methodisprivate(name))
                        {
                            return nn_exceptions_throw(state, "cannot call private module method '%s'", name->sbuf->data);
                        }
                        return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
                    }
                    return nn_exceptions_throw(state, "module %s does not define class or method %s()", module->name, name->sbuf->data);
                }
                break;
            case NEON_OBJTYPE_CLASS:
                {
                    NEON_APIDEBUG(state, "receiver is a class");
                    klass = nn_value_asclass(receiver);
                    field = nn_table_getfieldbyostr(klass->methods, name);
                    if(field != NULL)
                    {
                        if(nn_value_getmethodtype(field->value) == NEON_FUNCTYPE_PRIVATE)
                        {
                            return nn_exceptions_throw(state, "cannot call private method %s() on %s", name->sbuf->data, klass->name->sbuf->data);
                        }
                        return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
                    }
                    else
                    {
                        field = nn_class_getstaticproperty(klass, name);
                        if(field != NULL)
                        {
                            return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
                        }
                        field = nn_class_getstaticmethodfield(klass, name);
                        if(field != NULL)
                        {
                            return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
                        }
                    }
                    return nn_exceptions_throw(state, "unknown method %s() in class %s", name->sbuf->data, klass->name->sbuf->data);
                }
            case NEON_OBJTYPE_INSTANCE:
                {
                    NNObjInstance* instance;
                    NEON_APIDEBUG(state, "receiver is an instance");
                    instance = nn_value_asinstance(receiver);
                    field = nn_table_getfieldbyostr(instance->properties, name);
                    if(field != NULL)
                    {
                        spos = (state->vmstate.stackidx + (-argcount - 1));
                        #if 0
                            state->vmstate.stackvalues[spos] = field->value;
                        #else
                            state->vmstate.stackvalues[spos] = receiver;
                        #endif
                        return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
                    }
                    return nn_vmutil_invokemethodfromclass(state, instance->klass, name, argcount);
                }
                break;
            case NEON_OBJTYPE_DICT:
                {
                    NEON_APIDEBUG(state, "receiver is a dictionary");
                    field = nn_class_getmethodfield(state->classprimdict, name);
                    if(field != NULL)
                    {
                        return nn_vm_callnative(state, nn_value_asfuncnative(field->value), receiver, argcount);
                    }
                    /* NEW in v0.0.84, dictionaries can declare extra methods as part of their entries. */
                    else
                    {
                        field = nn_table_getfieldbyostr(nn_value_asdict(receiver)->htab, name);
                        if(field != NULL)
                        {
                            if(nn_value_iscallable(field->value))
                            {
                                return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
                            }
                        }
                    }
                    return nn_exceptions_throw(state, "'dict' has no method %s()", name->sbuf->data);
                }
                default:
                    {
                    }
                    break;
        }
    }
    klass = nn_value_getclassfor(state, receiver);
    if(klass == NULL)
    {
        /* @TODO: have methods for non objects as well. */
        return nn_exceptions_throw(state, "non-object %s has no method named '%s'", nn_value_typename(receiver), name->sbuf->data);
    }
    field = nn_class_getmethodfield(klass, name);
    if(field != NULL)
    {
        return nn_vm_callvaluewithobject(state, field->value, receiver, argcount);
    }
    return nn_exceptions_throw(state, "'%s' has no method %s()", klass->name->sbuf->data, name->sbuf->data);
}

NEON_FORCEINLINE bool nn_vmutil_bindmethod(NNState* state, NNObjClass* klass, NNObjString* name)
{
    NNValue val;
    NNProperty* field;
    NNObjFuncBound* bound;
    field = nn_table_getfieldbyostr(klass->methods, name);
    if(field != NULL)
    {
        if(nn_value_getmethodtype(field->value) == NEON_FUNCTYPE_PRIVATE)
        {
            return nn_exceptions_throw(state, "cannot get private property '%s' from instance", name->sbuf->data);
        }
        val = nn_vmbits_stackpeek(state, 0);
        bound = nn_object_makefuncbound(state, val, nn_value_asfuncclosure(field->value));
        nn_vmbits_stackpop(state);
        nn_vmbits_stackpush(state, nn_value_fromobject(bound));
        return true;
    }
    return nn_exceptions_throw(state, "undefined property '%s'", name->sbuf->data);
}

NEON_FORCEINLINE NNObjUpvalue* nn_vmutil_upvaluescapture(NNState* state, NNValue* local, int stackpos)
{
    NNObjUpvalue* upvalue;
    NNObjUpvalue* prevupvalue;
    NNObjUpvalue* createdupvalue;
    prevupvalue = NULL;
    upvalue = state->vmstate.openupvalues;
    while(upvalue != NULL && (&upvalue->location) > local)
    {
        prevupvalue = upvalue;
        upvalue = upvalue->next;
    }
    if(upvalue != NULL && (&upvalue->location) == local)
    {
        return upvalue;
    }
    createdupvalue = nn_object_makeupvalue(state, local, stackpos);
    createdupvalue->next = upvalue;
    if(prevupvalue == NULL)
    {
        state->vmstate.openupvalues = createdupvalue;
    }
    else
    {
        prevupvalue->next = createdupvalue;
    }
    return createdupvalue;
}

NEON_FORCEINLINE void nn_vmutil_upvaluesclose(NNState* state, const NNValue* last)
{
    NNObjUpvalue* upvalue;
    while(state->vmstate.openupvalues != NULL && (&state->vmstate.openupvalues->location) >= last)
    {
        upvalue = state->vmstate.openupvalues;
        upvalue->closed = upvalue->location;
        upvalue->location = upvalue->closed;
        state->vmstate.openupvalues = upvalue->next;
    }
}

NEON_FORCEINLINE void nn_vmutil_definemethod(NNState* state, NNObjString* name)
{
    NNValue method;
    NNObjClass* klass;
    method = nn_vmbits_stackpeek(state, 0);
    klass = nn_value_asclass(nn_vmbits_stackpeek(state, 1));
    nn_table_set(klass->methods, nn_value_fromobject(name), method);
    if(nn_value_getmethodtype(method) == NEON_FUNCTYPE_INITIALIZER)
    {
        klass->constructor = method;
    }
    nn_vmbits_stackpop(state);
}

NEON_FORCEINLINE void nn_vmutil_defineproperty(NNState* state, NNObjString* name, bool isstatic)
{
    NNValue property;
    NNObjClass* klass;
    property = nn_vmbits_stackpeek(state, 0);
    klass = nn_value_asclass(nn_vmbits_stackpeek(state, 1));
    if(!isstatic)
    {
        nn_class_defproperty(klass, name->sbuf->data, property);
    }
    else
    {
        nn_class_setstaticproperty(klass, name, property);
    }
    nn_vmbits_stackpop(state);
}

bool nn_value_isfalse(NNValue value)
{
    if(nn_value_isbool(value))
    {
        return nn_value_isbool(value) && !nn_value_asbool(value);
    }
    if(nn_value_isnull(value) || nn_value_isempty(value))
    {
        return true;
    }
    /* -1 is the number equivalent of false */
    if(nn_value_isnumber(value))
    {
        return nn_value_asnumber(value) < 0;
    }
    /* Non-empty strings are true, empty strings are false.*/
    if(nn_value_isstring(value))
    {
        return nn_value_asstring(value)->sbuf->length < 1;
    }
    /* Non-empty lists are true, empty lists are false.*/
    if(nn_value_isarray(value))
    {
        return nn_value_asarray(value)->varray->listcount == 0;
    }
    /* Non-empty dicts are true, empty dicts are false. */
    if(nn_value_isdict(value))
    {
        return nn_value_asdict(value)->names->listcount == 0;
    }
    /*
    // All classes are true
    // All closures are true
    // All bound methods are true
    // All functions are in themselves true if you do not account for what they
    // return.
    */
    return false;
}

bool nn_util_isinstanceof(NNObjClass* klass1, NNObjClass* expected)
{
    size_t klen;
    size_t elen;
    const char* kname;
    const char* ename;
    while(klass1 != NULL)
    {
        elen = expected->name->sbuf->length;
        klen = klass1->name->sbuf->length;
        ename = expected->name->sbuf->data;
        kname = klass1->name->sbuf->data;
        if(elen == klen && memcmp(kname, ename, klen) == 0)
        {
            return true;
        }
        klass1 = klass1->superclass;
    }
    return false;
}

bool nn_dict_setentry(NNObjDict* dict, NNValue key, NNValue value)
{
    NNValue tempvalue;
    if(!nn_table_get(dict->htab, key, &tempvalue))
    {
        /* add key if it doesn't exist. */
        nn_vallist_push(dict->names, key);
    }
    return nn_table_set(dict->htab, key, value);
}

void nn_dict_addentry(NNObjDict* dict, NNValue key, NNValue value)
{
    nn_dict_setentry(dict, key, value);
}

void nn_dict_addentrycstr(NNObjDict* dict, const char* ckey, NNValue value)
{
    NNObjString* os;
    NNState* state;
    state = ((NNObject*)dict)->pvm;
    os = nn_string_copycstr(state, ckey);
    nn_dict_addentry(dict, nn_value_fromobject(os), value);
}

NNProperty* nn_dict_getentry(NNObjDict* dict, NNValue key)
{
    return nn_table_getfield(dict->htab, key);
}

NEON_FORCEINLINE NNObjString* nn_vmutil_multiplystring(NNState* state, NNObjString* str, double number)
{
    size_t i;
    size_t times;
    NNPrinter pr;
    times = (size_t)number;
    /* 'str' * 0 == '', 'str' * -1 == '' */
    if(times <= 0)
    {
        return nn_string_copylen(state, "", 0);
    }
    /* 'str' * 1 == 'str' */
    else if(times == 1)
    {
        return str;
    }
    nn_printer_makestackstring(state, &pr);
    for(i = 0; i < times; i++)
    {
        nn_printer_writestringl(&pr, str->sbuf->data, str->sbuf->length);
    }
    return nn_printer_takestring(&pr);
}

NEON_FORCEINLINE NNObjArray* nn_vmutil_combinearrays(NNState* state, NNObjArray* a, NNObjArray* b)
{
    size_t i;
    NNObjArray* list;
    list = nn_object_makearray(state);
    nn_vmbits_stackpush(state, nn_value_fromobject(list));
    for(i = 0; i < a->varray->listcount; i++)
    {
        nn_vallist_push(list->varray, a->varray->listitems[i]);
    }
    for(i = 0; i < b->varray->listcount; i++)
    {
        nn_vallist_push(list->varray, b->varray->listitems[i]);
    }
    nn_vmbits_stackpop(state);
    return list;
}

NEON_FORCEINLINE void nn_vmutil_multiplyarray(NNState* state, NNObjArray* from, NNObjArray* newlist, size_t times)
{
    size_t i;
    size_t j;
    (void)state;
    for(i = 0; i < times; i++)
    {
        for(j = 0; j < from->varray->listcount; j++)
        {
            nn_vallist_push(newlist->varray, from->varray->listitems[j]);
        }
    }
}

NEON_FORCEINLINE bool nn_vmutil_dogetrangedindexofarray(NNState* state, NNObjArray* list, bool willassign)
{
    long i;
    long idxlower;
    long idxupper;
    NNValue valupper;
    NNValue vallower;
    NNObjArray* newlist;
    valupper = nn_vmbits_stackpeek(state, 0);
    vallower = nn_vmbits_stackpeek(state, 1);
    if(!(nn_value_isnull(vallower) || nn_value_isnumber(vallower)) || !(nn_value_isnumber(valupper) || nn_value_isnull(valupper)))
    {
        nn_vmbits_stackpopn(state, 2);
        return nn_exceptions_throw(state, "list range index expects upper and lower to be numbers, but got '%s', '%s'", nn_value_typename(vallower), nn_value_typename(valupper));
    }
    idxlower = 0;
    if(nn_value_isnumber(vallower))
    {
        idxlower = nn_value_asnumber(vallower);
    }
    if(nn_value_isnull(valupper))
    {
        idxupper = list->varray->listcount;
    }
    else
    {
        idxupper = nn_value_asnumber(valupper);
    }
    if((idxlower < 0) || ((idxupper < 0) && ((long)(list->varray->listcount + idxupper) < 0)) || (idxlower >= (long)list->varray->listcount))
    {
        /* always return an empty list... */
        if(!willassign)
        {
            /* +1 for the list itself */
            nn_vmbits_stackpopn(state, 3);
        }
        nn_vmbits_stackpush(state, nn_value_fromobject(nn_object_makearray(state)));
        return true;
    }
    if(idxupper < 0)
    {
        idxupper = list->varray->listcount + idxupper;
    }
    if(idxupper > (long)list->varray->listcount)
    {
        idxupper = list->varray->listcount;
    }
    newlist = nn_object_makearray(state);
    nn_vmbits_stackpush(state, nn_value_fromobject(newlist));
    for(i = idxlower; i < idxupper; i++)
    {
        nn_vallist_push(newlist->varray, list->varray->listitems[i]);
    }
    /* clear gc protect */
    nn_vmbits_stackpop(state);
    if(!willassign)
    {
        /* +1 for the list itself */
        nn_vmbits_stackpopn(state, 3);
    }
    nn_vmbits_stackpush(state, nn_value_fromobject(newlist));
    return true;
}

NEON_FORCEINLINE bool nn_vmutil_dogetrangedindexofstring(NNState* state, NNObjString* string, bool willassign)
{
    int end;
    int start;
    int length;
    int idxupper;
    int idxlower;
    NNValue valupper;
    NNValue vallower;
    valupper = nn_vmbits_stackpeek(state, 0);
    vallower = nn_vmbits_stackpeek(state, 1);
    if(!(nn_value_isnull(vallower) || nn_value_isnumber(vallower)) || !(nn_value_isnumber(valupper) || nn_value_isnull(valupper)))
    {
        nn_vmbits_stackpopn(state, 2);
        return nn_exceptions_throw(state, "string range index expects upper and lower to be numbers, but got '%s', '%s'", nn_value_typename(vallower), nn_value_typename(valupper));
    }
    length = string->sbuf->length;
    idxlower = 0;
    if(nn_value_isnumber(vallower))
    {
        idxlower = nn_value_asnumber(vallower);
    }
    if(nn_value_isnull(valupper))
    {
        idxupper = length;
    }
    else
    {
        idxupper = nn_value_asnumber(valupper);
    }
    if(idxlower < 0 || (idxupper < 0 && ((length + idxupper) < 0)) || idxlower >= length)
    {
        /* always return an empty string... */
        if(!willassign)
        {
            /* +1 for the string itself */
            nn_vmbits_stackpopn(state, 3);
        }
        nn_vmbits_stackpush(state, nn_value_fromobject(nn_string_copylen(state, "", 0)));
        return true;
    }
    if(idxupper < 0)
    {
        idxupper = length + idxupper;
    }
    if(idxupper > length)
    {
        idxupper = length;
    }
    start = idxlower;
    end = idxupper;
    if(!willassign)
    {
        /* +1 for the string itself */
        nn_vmbits_stackpopn(state, 3);
    }
    nn_vmbits_stackpush(state, nn_value_fromobject(nn_string_copylen(state, string->sbuf->data + start, end - start)));
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_getrangedindex(NNState* state)
{
    bool isgotten;
    uint8_t willassign;
    NNValue vfrom;
    willassign = nn_vmbits_readbyte(state);
    isgotten = true;
    vfrom = nn_vmbits_stackpeek(state, 2);
    if(nn_value_isobject(vfrom))
    {
        switch(nn_value_asobject(vfrom)->type)
        {
            case NEON_OBJTYPE_STRING:
            {
                if(!nn_vmutil_dogetrangedindexofstring(state, nn_value_asstring(vfrom), willassign))
                {
                    return false;
                }
                break;
            }
            case NEON_OBJTYPE_ARRAY:
            {
                if(!nn_vmutil_dogetrangedindexofarray(state, nn_value_asarray(vfrom), willassign))
                {
                    return false;
                }
                break;
            }
            default:
            {
                isgotten = false;
                break;
            }
        }
    }
    else
    {
        isgotten = false;
    }
    if(!isgotten)
    {
        return nn_exceptions_throw(state, "cannot range index object of type %s", nn_value_typename(vfrom));
    }
    return true;
}

NEON_FORCEINLINE bool nn_vmutil_doindexgetdict(NNState* state, NNObjDict* dict, bool willassign)
{
    NNValue vindex;
    NNProperty* field;
    vindex = nn_vmbits_stackpeek(state, 0);
    field = nn_dict_getentry(dict, vindex);
    if(field != NULL)
    {
        if(!willassign)
        {
            /* we can safely get rid of the index from the stack */
            nn_vmbits_stackpopn(state, 2);
        }
        nn_vmbits_stackpush(state, field->value);
        return true;
    }
    nn_vmbits_stackpopn(state, 1);
    nn_vmbits_stackpush(state, nn_value_makeempty());
    return true;
}

NEON_FORCEINLINE bool nn_vmutil_doindexgetmodule(NNState* state, NNObjModule* module, bool willassign)
{
    NNValue vindex;
    NNValue result;
    vindex = nn_vmbits_stackpeek(state, 0);
    if(nn_table_get(module->deftable, vindex, &result))
    {
        if(!willassign)
        {
            /* we can safely get rid of the index from the stack */
            nn_vmbits_stackpopn(state, 2);
        }
        nn_vmbits_stackpush(state, result);
        return true;
    }
    nn_vmbits_stackpop(state);
    return nn_exceptions_throw(state, "%s is undefined in module %s", nn_value_tostring(state, vindex)->sbuf->data, module->name);
}

NEON_FORCEINLINE bool nn_vmutil_doindexgetstring(NNState* state, NNObjString* string, bool willassign)
{
    int end;
    int start;
    int index;
    int maxlength;
    int realindex;
    NNValue vindex;
    NNObjRange* rng;
    (void)realindex;
    vindex = nn_vmbits_stackpeek(state, 0);
    if(!nn_value_isnumber(vindex))
    {
        if(nn_value_isrange(vindex))
        {
            rng = nn_value_asrange(vindex);
            nn_vmbits_stackpop(state);
            nn_vmbits_stackpush(state, nn_value_makenumber(rng->lower));
            nn_vmbits_stackpush(state, nn_value_makenumber(rng->upper));
            return nn_vmutil_dogetrangedindexofstring(state, string, willassign);
        }
        nn_vmbits_stackpopn(state, 1);
        return nn_exceptions_throw(state, "strings are numerically indexed");
    }
    index = nn_value_asnumber(vindex);
    maxlength = string->sbuf->length;
    realindex = index;
    if(index < 0)
    {
        index = maxlength + index;
    }
    if(index < maxlength && index >= 0)
    {
        start = index;
        end = index + 1;
        if(!willassign)
        {
            /*
            // we can safely get rid of the index from the stack
            // +1 for the string itself
            */
            nn_vmbits_stackpopn(state, 2);
        }
        nn_vmbits_stackpush(state, nn_value_fromobject(nn_string_copylen(state, string->sbuf->data + start, end - start)));
        return true;
    }
    nn_vmbits_stackpopn(state, 1);
    #if 0
        return nn_exceptions_throw(state, "string index %d out of range of %d", realindex, maxlength);
    #else
        nn_vmbits_stackpush(state, nn_value_makeempty());
        return true;
    #endif
}

NEON_FORCEINLINE bool nn_vmutil_doindexgetarray(NNState* state, NNObjArray* list, bool willassign)
{
    long index;
    NNValue finalval;
    NNValue vindex;
    NNObjRange* rng;
    vindex = nn_vmbits_stackpeek(state, 0);
    if(NEON_UNLIKELY(!nn_value_isnumber(vindex)))
    {
        if(nn_value_isrange(vindex))
        {
            rng = nn_value_asrange(vindex);
            nn_vmbits_stackpop(state);
            nn_vmbits_stackpush(state, nn_value_makenumber(rng->lower));
            nn_vmbits_stackpush(state, nn_value_makenumber(rng->upper));
            return nn_vmutil_dogetrangedindexofarray(state, list, willassign);
        }
        nn_vmbits_stackpop(state);
        return nn_exceptions_throw(state, "list are numerically indexed");
    }
    index = nn_value_asnumber(vindex);
    if(NEON_UNLIKELY(index < 0))
    {
        index = list->varray->listcount + index;
    }
    if((index < (long)list->varray->listcount) && (index >= 0))
    {
        finalval = list->varray->listitems[index];
    }
    else
    {
        finalval = nn_value_makenull();
    }
    if(!willassign)
    {
        /*
        // we can safely get rid of the index from the stack
        // +1 for the list itself
        */
        nn_vmbits_stackpopn(state, 2);
    }
    nn_vmbits_stackpush(state, finalval);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_indexget(NNState* state)
{
    bool isgotten;
    uint8_t willassign;
    NNValue peeked;
    willassign = nn_vmbits_readbyte(state);
    isgotten = true;
    peeked = nn_vmbits_stackpeek(state, 1);
    if(NEON_LIKELY(nn_value_isobject(peeked)))
    {
        switch(nn_value_asobject(peeked)->type)
        {
            case NEON_OBJTYPE_STRING:
            {
                if(!nn_vmutil_doindexgetstring(state, nn_value_asstring(peeked), willassign))
                {
                    return false;
                }
                break;
            }
            case NEON_OBJTYPE_ARRAY:
            {
                if(!nn_vmutil_doindexgetarray(state, nn_value_asarray(peeked), willassign))
                {
                    return false;
                }
                break;
            }
            case NEON_OBJTYPE_DICT:
            {
                if(!nn_vmutil_doindexgetdict(state, nn_value_asdict(peeked), willassign))
                {
                    return false;
                }
                break;
            }
            case NEON_OBJTYPE_MODULE:
            {
                if(!nn_vmutil_doindexgetmodule(state, nn_value_asmodule(peeked), willassign))
                {
                    return false;
                }
                break;
            }
            default:
            {
                isgotten = false;
                break;
            }
        }
    }
    else
    {
        isgotten = false;
    }
    if(!isgotten)
    {
        nn_exceptions_throw(state, "cannot index object of type %s", nn_value_typename(peeked));
    }
    return true;
}


NEON_FORCEINLINE bool nn_vmutil_dosetindexdict(NNState* state, NNObjDict* dict, NNValue index, NNValue value)
{
    nn_dict_setentry(dict, index, value);
    /* pop the value, index and dict out */
    nn_vmbits_stackpopn(state, 3);
    /*
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    */
    nn_vmbits_stackpush(state, value);
    return true;
}

NEON_FORCEINLINE bool nn_vmutil_dosetindexmodule(NNState* state, NNObjModule* module, NNValue index, NNValue value)
{
    nn_table_set(module->deftable, index, value);
    /* pop the value, index and dict out */
    nn_vmbits_stackpopn(state, 3);
    /*
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    */
    nn_vmbits_stackpush(state, value);
    return true;
}

NEON_FORCEINLINE bool nn_vmutil_doindexsetarray(NNState* state, NNObjArray* list, NNValue index, NNValue value)
{
    int tmp;
    int rawpos;
    int position;
    int ocnt;
    int ocap;
    int vasz;
    if(NEON_UNLIKELY(!nn_value_isnumber(index)))
    {
        nn_vmbits_stackpopn(state, 3);
        /* pop the value, index and list out */
        return nn_exceptions_throw(state, "list are numerically indexed");
    }
    ocap = list->varray->listcapacity;
    ocnt = list->varray->listcount;
    rawpos = nn_value_asnumber(index);
    position = rawpos;
    if(rawpos < 0)
    {
        rawpos = list->varray->listcount + rawpos;
    }
    if(position < ocap && position > -(ocap))
    {
        list->varray->listitems[position] = value;
        if(position >= ocnt)
        {
            list->varray->listcount++;
        }
    }
    else
    {
        if(position < 0)
        {
            fprintf(stderr, "inverting negative position %d\n", position);
            position = (~position) + 1;
        }
        tmp = 0;
        vasz = list->varray->listcount;
        if((position > vasz) || ((position == 0) && (vasz == 0)))
        {
            if(position == 0)
            {
                nn_array_push(list, nn_value_makeempty());
            }
            else
            {
                tmp = position + 1;
                while(tmp > vasz)
                {
                    nn_array_push(list, nn_value_makeempty());
                    tmp--;
                }
            }
        }
        fprintf(stderr, "setting value at position %d (array count: %ld)\n", position, list->varray->listcount);
    }
    list->varray->listitems[position] = value;
    /* pop the value, index and list out */
    nn_vmbits_stackpopn(state, 3);
    /*
    // leave the value on the stack for consumption
    // e.g. variable = list[index] = 10    
    */
    nn_vmbits_stackpush(state, value);
    return true;
    /*
    // pop the value, index and list out
    //nn_vmbits_stackpopn(state, 3);
    //return nn_exceptions_throw(state, "lists index %d out of range", rawpos);
    //nn_vmbits_stackpush(state, nn_value_makeempty());
    //return true;
    */
}

NEON_FORCEINLINE bool nn_vmutil_dosetindexstring(NNState* state, NNObjString* os, NNValue index, NNValue value)
{
    int iv;
    int rawpos;
    int position;
    int oslen;
    if(!nn_value_isnumber(index))
    {
        nn_vmbits_stackpopn(state, 3);
        /* pop the value, index and list out */
        return nn_exceptions_throw(state, "strings are numerically indexed");
    }
    iv = nn_value_asnumber(value);
    rawpos = nn_value_asnumber(index);
    oslen = os->sbuf->length;
    position = rawpos;
    if(rawpos < 0)
    {
        position = (oslen + rawpos);
    }
    if(position < oslen && position > -oslen)
    {
        os->sbuf->data[position] = iv;
        /* pop the value, index and list out */
        nn_vmbits_stackpopn(state, 3);
        /*
        // leave the value on the stack for consumption
        // e.g. variable = list[index] = 10
        */
        nn_vmbits_stackpush(state, value);
        return true;
    }
    else
    {
        dyn_strbuf_appendchar(os->sbuf, iv);
        nn_vmbits_stackpopn(state, 3);
        nn_vmbits_stackpush(state, value);
    }
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_indexset(NNState* state)
{
    bool isset;
    NNValue value;
    NNValue index;
    NNValue target;
    isset = true;
    target = nn_vmbits_stackpeek(state, 2);
    if(NEON_LIKELY(nn_value_isobject(target)))
    {
        value = nn_vmbits_stackpeek(state, 0);
        index = nn_vmbits_stackpeek(state, 1);
        if(NEON_UNLIKELY(nn_value_isempty(value)))
        {
            return nn_exceptions_throw(state, "empty cannot be assigned");
        }
        switch(nn_value_asobject(target)->type)
        {
            case NEON_OBJTYPE_ARRAY:
                {
                    if(!nn_vmutil_doindexsetarray(state, nn_value_asarray(target), index, value))
                    {
                        return false;
                    }
                }
                break;
            case NEON_OBJTYPE_STRING:
                {
                    if(!nn_vmutil_dosetindexstring(state, nn_value_asstring(target), index, value))
                    {
                        return false;
                    }
                }
                break;
            case NEON_OBJTYPE_DICT:
                {
                    return nn_vmutil_dosetindexdict(state, nn_value_asdict(target), index, value);
                }
                break;
            case NEON_OBJTYPE_MODULE:
                {
                    return nn_vmutil_dosetindexmodule(state, nn_value_asmodule(target), index, value);
                }
                break;
            default:
                {
                    isset = false;
                }
                break;
        }
    }
    else
    {
        isset = false;
    }
    if(!isset)
    {
        return nn_exceptions_throw(state, "type of %s is not a valid iterable", nn_value_typename(nn_vmbits_stackpeek(state, 3)));
    }
    return true;
}

NEON_FORCEINLINE bool nn_vmutil_concatenate(NNState* state)
{
    NNValue vleft;
    NNValue vright;
    NNPrinter pr;
    NNObjString* result;
    vright = nn_vmbits_stackpeek(state, 0);
    vleft = nn_vmbits_stackpeek(state, 1);
    nn_printer_makestackstring(state, &pr);
    nn_printer_printvalue(&pr, vleft, false, true);
    nn_printer_printvalue(&pr, vright, false, true);
    result = nn_printer_takestring(&pr);
    nn_vmbits_stackpopn(state, 2);
    nn_vmbits_stackpush(state, nn_value_fromobject(result));
    return true;
}

NEON_FORCEINLINE double nn_vmutil_floordiv(double a, double b)
{
    int d;
    d = (int)a / (int)b;
    return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

NEON_FORCEINLINE double nn_vmutil_modulo(double a, double b)
{
    double r;
    r = fmod(a, b);
    if(r != 0 && ((r < 0) != (b < 0)))
    {
        r += b;
    }
    return r;
}

NEON_FORCEINLINE NNProperty* nn_vmutil_getproperty(NNState* state, NNValue peeked, NNObjString* name)
{
    NNProperty* field;
    switch(nn_value_asobject(peeked)->type)
    {
        case NEON_OBJTYPE_MODULE:
            {
                NNObjModule* module;
                module = nn_value_asmodule(peeked);
                field = nn_table_getfieldbyostr(module->deftable, name);
                if(field != NULL)
                {
                    if(nn_util_methodisprivate(name))
                    {
                        nn_exceptions_throw(state, "cannot get private module property '%s'", name->sbuf->data);
                        return NULL;
                    }
                    return field;
                }
                nn_exceptions_throw(state, "%s module does not define '%s'", module->name, name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_CLASS:
            {
                field = nn_table_getfieldbyostr(nn_value_asclass(peeked)->methods, name);
                if(field != NULL)
                {
                    if(nn_value_getmethodtype(field->value) == NEON_FUNCTYPE_STATIC)
                    {
                        if(nn_util_methodisprivate(name))
                        {
                            nn_exceptions_throw(state, "cannot call private property '%s' of class %s", name->sbuf->data,
                                nn_value_asclass(peeked)->name->sbuf->data);
                            return NULL;
                        }
                        return field;
                    }
                }
                else
                {
                    field = nn_class_getstaticproperty(nn_value_asclass(peeked), name);
                    if(field != NULL)
                    {
                        if(nn_util_methodisprivate(name))
                        {
                            nn_exceptions_throw(state, "cannot call private property '%s' of class %s", name->sbuf->data,
                                nn_value_asclass(peeked)->name->sbuf->data);
                            return NULL;
                        }
                        return field;
                    }
                }
                nn_exceptions_throw(state, "class %s does not have a static property or method named '%s'",
                    nn_value_asclass(peeked)->name->sbuf->data, name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_INSTANCE:
            {
                NNObjInstance* instance;
                instance = nn_value_asinstance(peeked);
                field = nn_table_getfieldbyostr(instance->properties, name);
                if(field != NULL)
                {
                    if(nn_util_methodisprivate(name))
                    {
                        nn_exceptions_throw(state, "cannot call private property '%s' from instance of %s", name->sbuf->data, instance->klass->name->sbuf->data);
                        return NULL;
                    }
                    return field;
                }
                if(nn_util_methodisprivate(name))
                {
                    nn_exceptions_throw(state, "cannot bind private property '%s' to instance of %s", name->sbuf->data, instance->klass->name->sbuf->data);
                    return NULL;
                }
                if(nn_vmutil_bindmethod(state, instance->klass, name))
                {
                    return field;
                }
                nn_exceptions_throw(state, "instance of class %s does not have a property or method named '%s'",
                    nn_value_asinstance(peeked)->klass->name->sbuf->data, name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_STRING:
            {
                field = nn_class_getpropertyfield(state->classprimstring, name);
                if(field != NULL)
                {
                    return field;
                }
                nn_exceptions_throw(state, "class String has no named property '%s'", name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_ARRAY:
            {
                field = nn_class_getpropertyfield(state->classprimarray, name);
                if(field != NULL)
                {
                    return field;
                }
                nn_exceptions_throw(state, "class Array has no named property '%s'", name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_RANGE:
            {
                field = nn_class_getpropertyfield(state->classprimrange, name);
                if(field != NULL)
                {
                    return field;
                }
                nn_exceptions_throw(state, "class Range has no named property '%s'", name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_DICT:
            {
                field = nn_table_getfieldbyostr(nn_value_asdict(peeked)->htab, name);
                if(field == NULL)
                {
                    field = nn_class_getpropertyfield(state->classprimdict, name);
                }
                if(field != NULL)
                {
                    return field;
                }
                nn_exceptions_throw(state, "unknown key or class Dict property '%s'", name->sbuf->data);
                return NULL;
            }
            break;
        case NEON_OBJTYPE_FILE:
            {
                field = nn_class_getpropertyfield(state->classprimfile, name);
                if(field != NULL)
                {
                    return field;
                }
                nn_exceptions_throw(state, "class File has no named property '%s'", name->sbuf->data);
                return NULL;
            }
            break;
        default:
            {
                nn_exceptions_throw(state, "object of type %s does not carry properties", nn_value_typename(peeked));
                return NULL;
            }
            break;
    }
    return NULL;
}

NEON_FORCEINLINE bool nn_vmdo_propertyget(NNState* state)
{
    NNValue peeked;
    NNProperty* field;
    NNObjString* name;
    name = nn_vmbits_readstring(state);
    peeked = nn_vmbits_stackpeek(state, 0);
    if(nn_value_isobject(peeked))
    {
        field = nn_vmutil_getproperty(state, peeked, name);
        if(field == NULL)
        {
            return false;
        }
        else
        {
            if(field->type == NEON_PROPTYPE_FUNCTION)
            {
                nn_vm_callvaluewithobject(state, field->value, peeked, 0);
            }
            else
            {
                nn_vmbits_stackpop(state);
                nn_vmbits_stackpush(state, field->value);
            }
        }
        return true;
    }
    else
    {
        nn_exceptions_throw(state, "'%s' of type %s does not have properties", nn_value_tostring(state, peeked)->sbuf->data,
            nn_value_typename(peeked));
    }
    return false;
}

NEON_FORCEINLINE bool nn_vmdo_propertygetself(NNState* state)
{
    NNValue peeked;
    NNObjString* name;
    NNObjClass* klass;
    NNObjInstance* instance;
    NNObjModule* module;
    NNProperty* field;
    name = nn_vmbits_readstring(state);
    peeked = nn_vmbits_stackpeek(state, 0);
    if(nn_value_isinstance(peeked))
    {
        instance = nn_value_asinstance(peeked);
        field = nn_table_getfieldbyostr(instance->properties, name);
        if(field != NULL)
        {
            /* pop the instance... */
            nn_vmbits_stackpop(state);
            nn_vmbits_stackpush(state, field->value);
            return true;
        }
        if(nn_vmutil_bindmethod(state, instance->klass, name))
        {
            return true;
        }
        nn_vmmac_tryraise(state, false, "instance of class %s does not have a property or method named '%s'",
            nn_value_asinstance(peeked)->klass->name->sbuf->data, name->sbuf->data);
        return false;
    }
    else if(nn_value_isclass(peeked))
    {
        klass = nn_value_asclass(peeked);
        field = nn_table_getfieldbyostr(klass->methods, name);
        if(field != NULL)
        {
            if(nn_value_getmethodtype(field->value) == NEON_FUNCTYPE_STATIC)
            {
                /* pop the class... */
                nn_vmbits_stackpop(state);
                nn_vmbits_stackpush(state, field->value);
                return true;
            }
        }
        else
        {
            field = nn_class_getstaticproperty(klass, name);
            if(field != NULL)
            {
                /* pop the class... */
                nn_vmbits_stackpop(state);
                nn_vmbits_stackpush(state, field->value);
                return true;
            }
        }
        nn_vmmac_tryraise(state, false, "class %s does not have a static property or method named '%s'", klass->name->sbuf->data, name->sbuf->data);
        return false;
    }
    else if(nn_value_ismodule(peeked))
    {
        module = nn_value_asmodule(peeked);
        field = nn_table_getfieldbyostr(module->deftable, name);
        if(field != NULL)
        {
            /* pop the module... */
            nn_vmbits_stackpop(state);
            nn_vmbits_stackpush(state, field->value);
            return true;
        }
        nn_vmmac_tryraise(state, false, "module %s does not define '%s'", module->name, name->sbuf->data);
        return false;
    }
    nn_vmmac_tryraise(state, false, "'%s' of type %s does not have properties", nn_value_tostring(state, peeked)->sbuf->data,
        nn_value_typename(peeked));
    return false;
}

NEON_FORCEINLINE bool nn_vmdo_propertyset(NNState* state)
{
    NNValue value;
    NNValue vtarget;
    NNValue vpeek;
    NNObjClass* klass;
    NNObjString* name;
    NNObjDict* dict;
    NNObjInstance* instance;
    vtarget = nn_vmbits_stackpeek(state, 1);
    if(!nn_value_isclass(vtarget) && !nn_value_isinstance(vtarget) && !nn_value_isdict(vtarget))
    {
        nn_exceptions_throw(state, "object of type %s cannot carry properties", nn_value_typename(vtarget));
        return false;
    }
    else if(nn_value_isempty(nn_vmbits_stackpeek(state, 0)))
    {
        nn_exceptions_throw(state, "empty cannot be assigned");
        return false;
    }
    name = nn_vmbits_readstring(state);
    vpeek = nn_vmbits_stackpeek(state, 0);
    if(nn_value_isclass(vtarget))
    {
        klass = nn_value_asclass(vtarget);
        if(nn_value_iscallable(vpeek))
        {
            nn_class_defmethod(state, klass, name->sbuf->data, vpeek);
        }
        else
        {
            nn_class_defproperty(klass, name->sbuf->data, vpeek);
        }
        value = nn_vmbits_stackpop(state);
        /* removing the class object */
        nn_vmbits_stackpop(state);
        nn_vmbits_stackpush(state, value);
    }
    else if(nn_value_isinstance(vtarget))
    {
        instance = nn_value_asinstance(vtarget);
        nn_instance_defproperty(instance, name->sbuf->data, vpeek);
        value = nn_vmbits_stackpop(state);
        /* removing the instance object */
        nn_vmbits_stackpop(state);
        nn_vmbits_stackpush(state, value);
    }
    else
    {
        dict = nn_value_asdict(vtarget);
        nn_dict_setentry(dict, nn_value_fromobject(name), vpeek);
        value = nn_vmbits_stackpop(state);
        /* removing the dictionary object */
        nn_vmbits_stackpop(state);
        nn_vmbits_stackpush(state, value);
    }
    return true;
}

NEON_FORCEINLINE double nn_vmutil_valtonum(NNValue v)
{
    if(nn_value_isnull(v))
    {
        return 0;
    }
    if(nn_value_isbool(v))
    {
        if(nn_value_asbool(v))
        {
            return 1;
        }
        return 0;
    }
    return nn_value_asnumber(v);
}


NEON_FORCEINLINE uint32_t nn_vmutil_valtouint(NNValue v)
{
    if(nn_value_isnull(v))
    {
        return 0;
    }
    if(nn_value_isbool(v))
    {
        if(nn_value_asbool(v))
        {
            return 1;
        }
        return 0;
    }
    return nn_value_asnumber(v);
}

NEON_FORCEINLINE long nn_vmutil_valtoint(NNValue v)
{
    return (long)nn_vmutil_valtonum(v);
}

NEON_FORCEINLINE bool nn_vmdo_dobinarydirect(NNState* state)
{
    bool isfail;
    long ibinright;
    long ibinleft;
    uint32_t ubinright;
    uint32_t ubinleft;
    double dbinright;
    double dbinleft;
    NNOpCode instruction;
    NNValue res;
    NNValue binvalleft;
    NNValue binvalright;
    instruction = (NNOpCode)state->vmstate.currentinstr.code;
    binvalright = nn_vmbits_stackpeek(state, 0);
    binvalleft = nn_vmbits_stackpeek(state, 1);
    isfail = (
        (!nn_value_isnumber(binvalright) && !nn_value_isbool(binvalright) && !nn_value_isnull(binvalright)) ||
        (!nn_value_isnumber(binvalleft) && !nn_value_isbool(binvalleft) && !nn_value_isnull(binvalleft))
    );
    if(isfail)
    {
        nn_vmmac_tryraise(state, false, "unsupported operand %s for %s and %s", nn_dbg_op2str(instruction), nn_value_typename(binvalleft), nn_value_typename(binvalright));
        return false;
    }
    binvalright = nn_vmbits_stackpop(state);
    binvalleft = nn_vmbits_stackpop(state);
    res = nn_value_makeempty();
    switch(instruction)
    {
        case NEON_OP_PRIMADD:
            {
                dbinright = nn_vmutil_valtonum(binvalright);
                dbinleft = nn_vmutil_valtonum(binvalleft);
                res = nn_value_makenumber(dbinleft + dbinright);
            }
            break;
        case NEON_OP_PRIMSUBTRACT:
            {
                dbinright = nn_vmutil_valtonum(binvalright);
                dbinleft = nn_vmutil_valtonum(binvalleft);
                res = nn_value_makenumber(dbinleft - dbinright);
            }
            break;
        case NEON_OP_PRIMDIVIDE:
            {
                dbinright = nn_vmutil_valtonum(binvalright);
                dbinleft = nn_vmutil_valtonum(binvalleft);
                res = nn_value_makenumber(dbinleft / dbinright);
            }
            break;
        case NEON_OP_PRIMMULTIPLY:
            {
                dbinright = nn_vmutil_valtonum(binvalright);
                dbinleft = nn_vmutil_valtonum(binvalleft);
                res = nn_value_makenumber(dbinleft * dbinright);
            }
            break;
        case NEON_OP_PRIMAND:
            {
                ibinright = nn_vmutil_valtoint(binvalright);
                ibinleft = nn_vmutil_valtoint(binvalleft);
                res = nn_value_makeint(ibinleft & ibinright);
            }
            break;
        case NEON_OP_PRIMOR:
            {
                ibinright = nn_vmutil_valtoint(binvalright);
                ibinleft = nn_vmutil_valtoint(binvalleft);
                res = nn_value_makeint(ibinleft | ibinright);
            }
            break;
        case NEON_OP_PRIMBITXOR:
            {
                ibinright = nn_vmutil_valtoint(binvalright);
                ibinleft = nn_vmutil_valtoint(binvalleft);
                res = nn_value_makeint(ibinleft ^ ibinright);
            }
            break;
        case NEON_OP_PRIMSHIFTLEFT:
            {
                /*
                via quickjs:
                    uint32_t v1;
                    uint32_t v2;
                    v1 = JS_VALUE_GET_INT(op1);
                    v2 = JS_VALUE_GET_INT(op2);
                    v2 &= 0x1f;
                    sp[-2] = JS_NewInt32(ctx, v1 << v2);
                */
                ubinright = nn_vmutil_valtouint(binvalright);
                ubinleft = nn_vmutil_valtouint(binvalleft);
                ubinright &= 0x1f;
                res = nn_value_makeint(ubinleft << ubinright);

            }
            break;
        case NEON_OP_PRIMSHIFTRIGHT:
            {
                /*
                    uint32_t v2;
                    v2 = JS_VALUE_GET_INT(op2);
                    v2 &= 0x1f;
                    sp[-2] = JS_NewUint32(ctx, (uint32_t)JS_VALUE_GET_INT(op1) >> v2);
                */
                ubinright = nn_vmutil_valtouint(binvalright);
                ubinleft = nn_vmutil_valtouint(binvalleft);
                ubinright &= 0x1f;
                res = nn_value_makeint(ubinleft >> ubinright);
            }
            break;
        case NEON_OP_PRIMGREATER:
            {
                dbinright = nn_vmutil_valtonum(binvalright);
                dbinleft = nn_vmutil_valtonum(binvalleft);
                res = nn_value_makebool(dbinleft > dbinright);
            }
            break;
        case NEON_OP_PRIMLESSTHAN:
            {
                dbinright = nn_vmutil_valtonum(binvalright);
                dbinleft = nn_vmutil_valtonum(binvalleft);
                res = nn_value_makebool(dbinleft < dbinright);
            }
            break;
        default:
            {
                fprintf(stderr, "unhandled instruction %d (%s)!\n", instruction, nn_dbg_op2str(instruction));
                return false;
            }
            break;
    }
    nn_vmbits_stackpush(state, res);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_globaldefine(NNState* state)
{
    NNValue val;
    NNObjString* name;
    NNHashTable* tab;
    name = nn_vmbits_readstring(state);
    val = nn_vmbits_stackpeek(state, 0);
    if(nn_value_isempty(val))
    {
        nn_vmmac_tryraise(state, false, "empty cannot be assigned");
        return false;
    }
    tab = state->vmstate.currentframe->closure->scriptfunc->module->deftable;
    nn_table_set(tab, nn_value_fromobject(name), val);
    nn_vmbits_stackpop(state);
    #if (defined(DEBUG_TABLE) && DEBUG_TABLE) || 0
    nn_table_print(state, state->debugwriter, state->globals, "globals");
    #endif
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_globalget(NNState* state)
{
    NNObjString* name;
    NNHashTable* tab;
    NNProperty* field;
    name = nn_vmbits_readstring(state);
    tab = state->vmstate.currentframe->closure->scriptfunc->module->deftable;
    field = nn_table_getfieldbyostr(tab, name);
    if(field == NULL)
    {
        field = nn_table_getfieldbyostr(state->globals, name);
        if(field == NULL)
        {
            nn_vmmac_tryraise(state, false, "global name '%s' is not defined", name->sbuf->data);
            return false;
        }
    }
    nn_vmbits_stackpush(state, field->value);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_globalset(NNState* state)
{
    NNObjString* name;
    NNHashTable* table;
    if(nn_value_isempty(nn_vmbits_stackpeek(state, 0)))
    {
        nn_vmmac_tryraise(state, false, "empty cannot be assigned");
        return false;
    }
    name = nn_vmbits_readstring(state);
    table = state->vmstate.currentframe->closure->scriptfunc->module->deftable;
    if(nn_table_set(table, nn_value_fromobject(name), nn_vmbits_stackpeek(state, 0)))
    {
        if(state->conf.enablestrictmode)
        {
            nn_table_delete(table, nn_value_fromobject(name));
            nn_vmmac_tryraise(state, false, "global name '%s' was not declared", name->sbuf->data);
            return false;
        }
    }
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_localget(NNState* state)
{
    size_t ssp;
    uint16_t slot;
    NNValue val;
    slot = nn_vmbits_readshort(state);
    ssp = state->vmstate.currentframe->stackslotpos;
    val = state->vmstate.stackvalues[ssp + slot];
    nn_vmbits_stackpush(state, val);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_localset(NNState* state)
{
    size_t ssp;
    uint16_t slot;
    NNValue peeked;
    slot = nn_vmbits_readshort(state);
    peeked = nn_vmbits_stackpeek(state, 0);
    if(nn_value_isempty(peeked))
    {
        nn_vmmac_tryraise(state, false, "empty cannot be assigned");
        return false;
    }
    ssp = state->vmstate.currentframe->stackslotpos;
    state->vmstate.stackvalues[ssp + slot] = peeked;
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_funcargget(NNState* state)
{
    size_t ssp;
    uint16_t slot;
    NNValue val;
    slot = nn_vmbits_readshort(state);
    ssp = state->vmstate.currentframe->stackslotpos;
    val = state->vmstate.stackvalues[ssp + slot];
    nn_vmbits_stackpush(state, val);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_funcargset(NNState* state)
{
    size_t ssp;
    uint16_t slot;
    NNValue peeked;
    slot = nn_vmbits_readshort(state);
    peeked = nn_vmbits_stackpeek(state, 0);
    if(nn_value_isempty(peeked))
    {
        nn_vmmac_tryraise(state, false, "empty cannot be assigned");
        return false;
    }
    ssp = state->vmstate.currentframe->stackslotpos;
    state->vmstate.stackvalues[ssp + slot] = peeked;
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_makeclosure(NNState* state)
{
    size_t i;
    int index;
    size_t ssp;
    uint8_t islocal;
    NNValue* upvals;
    NNObjFuncScript* function;
    NNObjFuncClosure* closure;
    function = nn_value_asfuncscript(nn_vmbits_readconst(state));
    closure = nn_object_makefuncclosure(state, function);
    nn_vmbits_stackpush(state, nn_value_fromobject(closure));
    for(i = 0; i < (size_t)closure->upvalcount; i++)
    {
        islocal = nn_vmbits_readbyte(state);
        index = nn_vmbits_readshort(state);
        if(islocal)
        {
            ssp = state->vmstate.currentframe->stackslotpos;
            upvals = &state->vmstate.stackvalues[ssp + index];
            closure->upvalues[i] = nn_vmutil_upvaluescapture(state, upvals, index);

        }
        else
        {
            closure->upvalues[i] = state->vmstate.currentframe->closure->upvalues[index];
        }
    }
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_makearray(NNState* state)
{
    int i;
    int count;
    NNObjArray* array;
    count = nn_vmbits_readshort(state);
    array = nn_object_makearray(state);
    state->vmstate.stackvalues[state->vmstate.stackidx + (-count - 1)] = nn_value_fromobject(array);
    for(i = count - 1; i >= 0; i--)
    {
        nn_array_push(array, nn_vmbits_stackpeek(state, i));
    }
    nn_vmbits_stackpopn(state, count);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_makedict(NNState* state)
{
    size_t i;
    size_t count;
    size_t realcount;
    NNValue name;
    NNValue value;
    NNObjDict* dict;
    /* 1 for key, 1 for value */
    realcount = nn_vmbits_readshort(state);
    count = realcount * 2;
    dict = nn_object_makedict(state);
    state->vmstate.stackvalues[state->vmstate.stackidx + (-count - 1)] = nn_value_fromobject(dict);
    for(i = 0; i < count; i += 2)
    {
        name = state->vmstate.stackvalues[state->vmstate.stackidx + (-count + i)];
        if(!nn_value_isstring(name) && !nn_value_isnumber(name) && !nn_value_isbool(name))
        {
            nn_vmmac_tryraise(state, false, "dictionary key must be one of string, number or boolean");
            return false;
        }
        value = state->vmstate.stackvalues[state->vmstate.stackidx + (-count + i + 1)];
        nn_dict_setentry(dict, name, value);
    }
    nn_vmbits_stackpopn(state, count);
    return true;
}

NEON_FORCEINLINE bool nn_vmdo_dobinaryfunc(NNState* state, const char* opname, nnbinopfunc_t op)
{
    double dbinright;
    double dbinleft;
    NNValue binvalright;
    NNValue binvalleft;
    binvalright = nn_vmbits_stackpeek(state, 0);
    binvalleft = nn_vmbits_stackpeek(state, 1);
    if((!nn_value_isnumber(binvalright) && !nn_value_isbool(binvalright))
    || (!nn_value_isnumber(binvalleft) && !nn_value_isbool(binvalleft)))
    {
        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "unsupported operand %s for %s and %s", opname, nn_value_typename(binvalleft), nn_value_typename(binvalright));
        return false;
    }
    binvalright = nn_vmbits_stackpop(state);
    dbinright = nn_value_isbool(binvalright) ? (nn_value_asbool(binvalright) ? 1 : 0) : nn_value_asnumber(binvalright);
    binvalleft = nn_vmbits_stackpop(state);
    dbinleft = nn_value_isbool(binvalleft) ? (nn_value_asbool(binvalleft) ? 1 : 0) : nn_value_asnumber(binvalleft);
    nn_vmbits_stackpush(state, nn_value_makenumber(op(dbinleft, dbinright)));
    return true;
}

NNStatus nn_vm_runvm(NNState* state, int exitframe, NNValue* rv)
{
    int iterpos;
    int printpos;
    int ofs;
    /*
    * this variable is a NOP; it only exists to ensure that functions outside of the
    * switch tree are not calling nn_vmmac_exitvm(), as its behavior could be undefined.
    */
    bool you_are_calling_exit_vm_outside_of_runvm;
    NNValue* dbgslot;
    NNInstruction currinstr;
    you_are_calling_exit_vm_outside_of_runvm = false;
    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
    while(true)
    {
        /*
        // try...finally... (i.e. try without a catch but finally
        // whose try body raises an exception)
        // can cause us to go into an invalid mode where frame count == 0
        // to fix this, we need to exit with an appropriate mode here.
        */
        if(state->vmstate.framecount == 0)
        {
            return NEON_STATUS_FAILRUNTIME;
        }
        if(nn_util_unlikely(state->conf.shoulddumpstack))
        {
            ofs = (int)(state->vmstate.currentframe->inscode - state->vmstate.currentframe->closure->scriptfunc->blob.instrucs);
            nn_dbg_printinstructionat(state->debugwriter, &state->vmstate.currentframe->closure->scriptfunc->blob, ofs);
            fprintf(stderr, "stack (before)=[\n");
            iterpos = 0;
            for(dbgslot = state->vmstate.stackvalues; dbgslot < &state->vmstate.stackvalues[state->vmstate.stackidx]; dbgslot++)
            {
                printpos = iterpos + 1;
                iterpos++;
                fprintf(stderr, "  [%s%d%s] ", nn_util_color(NEON_COLOR_YELLOW), printpos, nn_util_color(NEON_COLOR_RESET));
                nn_printer_writefmt(state->debugwriter, "%s", nn_util_color(NEON_COLOR_YELLOW));
                nn_printer_printvalue(state->debugwriter, *dbgslot, true, false);
                nn_printer_writefmt(state->debugwriter, "%s", nn_util_color(NEON_COLOR_RESET));
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "]\n");
        }
        currinstr = nn_vmbits_readinstruction(state);
        state->vmstate.currentinstr = currinstr;
        switch(currinstr.code)
        {
            case NEON_OP_RETURN:
                {
                    size_t ssp;
                    NNValue result;
                    result = nn_vmbits_stackpop(state);
                    if(rv != NULL)
                    {
                        *rv = result;
                    }
                    ssp = state->vmstate.currentframe->stackslotpos;
                    nn_vmutil_upvaluesclose(state, &state->vmstate.stackvalues[ssp]);
                    state->vmstate.framecount--;
                    if(state->vmstate.framecount == 0)
                    {
                        nn_vmbits_stackpop(state);
                        return NEON_STATUS_OK;
                    }
                    ssp = state->vmstate.currentframe->stackslotpos;
                    state->vmstate.stackidx = ssp;
                    nn_vmbits_stackpush(state, result);
                    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                    if(state->vmstate.framecount == (size_t)exitframe)
                    {
                        return NEON_STATUS_OK;
                    }
                }
                break;
            case NEON_OP_PUSHCONSTANT:
                {
                    NNValue constant;
                    constant = nn_vmbits_readconst(state);
                    nn_vmbits_stackpush(state, constant);
                }
                break;
            case NEON_OP_PRIMADD:
                {
                    NNValue valright;
                    NNValue valleft;
                    NNValue result;
                    valright = nn_vmbits_stackpeek(state, 0);
                    valleft = nn_vmbits_stackpeek(state, 1);
                    if(nn_value_isstring(valright) || nn_value_isstring(valleft))
                    {
                        if(nn_util_unlikely(!nn_vmutil_concatenate(state)))
                        {
                            nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "unsupported operand + for %s and %s", nn_value_typename(valleft), nn_value_typename(valright));
                            break;
                        }
                    }
                    else if(nn_value_isarray(valleft) && nn_value_isarray(valright))
                    {
                        result = nn_value_fromobject(nn_vmutil_combinearrays(state, nn_value_asarray(valleft), nn_value_asarray(valright)));
                        nn_vmbits_stackpopn(state, 2);
                        nn_vmbits_stackpush(state, result);
                    }
                    else
                    {
                        nn_vmdo_dobinarydirect(state);
                    }
                }
                break;
            case NEON_OP_PRIMSUBTRACT:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMMULTIPLY:
                {
                    int intnum;
                    double dbnum;
                    NNValue peekleft;
                    NNValue peekright;
                    NNValue result;
                    NNObjString* string;
                    NNObjArray* list;
                    NNObjArray* newlist;
                    peekright = nn_vmbits_stackpeek(state, 0);
                    peekleft = nn_vmbits_stackpeek(state, 1);
                    if(nn_value_isstring(peekleft) && nn_value_isnumber(peekright))
                    {
                        dbnum = nn_value_asnumber(peekright);
                        string = nn_value_asstring(nn_vmbits_stackpeek(state, 1));
                        result = nn_value_fromobject(nn_vmutil_multiplystring(state, string, dbnum));
                        nn_vmbits_stackpopn(state, 2);
                        nn_vmbits_stackpush(state, result);
                        break;
                    }
                    else if(nn_value_isarray(peekleft) && nn_value_isnumber(peekright))
                    {
                        intnum = (int)nn_value_asnumber(peekright);
                        nn_vmbits_stackpop(state);
                        list = nn_value_asarray(peekleft);
                        newlist = nn_object_makearray(state);
                        nn_vmbits_stackpush(state, nn_value_fromobject(newlist));
                        nn_vmutil_multiplyarray(state, list, newlist, intnum);
                        nn_vmbits_stackpopn(state, 2);
                        nn_vmbits_stackpush(state, nn_value_fromobject(newlist));
                        break;
                    }
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMDIVIDE:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMMODULO:
                {
                    if(nn_vmdo_dobinaryfunc(state, "%", (nnbinopfunc_t)nn_vmutil_modulo))
                    {
                    }
                }
                break;
            case NEON_OP_PRIMPOW:
                {
                    if(nn_vmdo_dobinaryfunc(state, "**", (nnbinopfunc_t)pow))
                    {
                    }
                }
                break;
            case NEON_OP_PRIMFLOORDIVIDE:
                {
                    if(nn_vmdo_dobinaryfunc(state, "//", (nnbinopfunc_t)nn_vmutil_floordiv))
                    {
                    }
                }
                break;
            case NEON_OP_PRIMNEGATE:
                {
                    NNValue peeked;
                    peeked = nn_vmbits_stackpeek(state, 0);
                    if(!nn_value_isnumber(peeked))
                    {
                        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "operator - not defined for object of type %s", nn_value_typename(peeked));
                        break;
                    }
                    nn_vmbits_stackpush(state, nn_value_makenumber(-nn_value_asnumber(nn_vmbits_stackpop(state))));
                }
                break;
            case NEON_OP_PRIMBITNOT:
            {
                NNValue peeked;
                peeked = nn_vmbits_stackpeek(state, 0);
                if(!nn_value_isnumber(peeked))
                {
                    nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "operator ~ not defined for object of type %s", nn_value_typename(peeked));
                    break;
                }
                nn_vmbits_stackpush(state, nn_value_makeint(~((int)nn_value_asnumber(nn_vmbits_stackpop(state)))));
                break;
            }
            case NEON_OP_PRIMAND:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMOR:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMBITXOR:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMSHIFTLEFT:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMSHIFTRIGHT:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PUSHONE:
                {
                    nn_vmbits_stackpush(state, nn_value_makenumber(1));
                }
                break;
            /* comparisons */
            case NEON_OP_EQUAL:
                {
                    NNValue a;
                    NNValue b;
                    b = nn_vmbits_stackpop(state);
                    a = nn_vmbits_stackpop(state);
                    nn_vmbits_stackpush(state, nn_value_makebool(nn_value_compare(state, a, b)));
                }
                break;
            case NEON_OP_PRIMGREATER:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMLESSTHAN:
                {
                    nn_vmdo_dobinarydirect(state);
                }
                break;
            case NEON_OP_PRIMNOT:
                {
                    nn_vmbits_stackpush(state, nn_value_makebool(nn_value_isfalse(nn_vmbits_stackpop(state))));
                }
                break;
            case NEON_OP_PUSHNULL:
                {
                    nn_vmbits_stackpush(state, nn_value_makenull());
                }
                break;
            case NEON_OP_PUSHEMPTY:
                {
                    nn_vmbits_stackpush(state, nn_value_makeempty());
                }
                break;
            case NEON_OP_PUSHTRUE:
                {
                    nn_vmbits_stackpush(state, nn_value_makebool(true));
                }
                break;
            case NEON_OP_PUSHFALSE:
                {
                    nn_vmbits_stackpush(state, nn_value_makebool(false));
                }
                break;

            case NEON_OP_JUMPNOW:
                {
                    uint16_t offset;
                    offset = nn_vmbits_readshort(state);
                    state->vmstate.currentframe->inscode += offset;
                }
                break;
            case NEON_OP_JUMPIFFALSE:
                {
                    uint16_t offset;
                    offset = nn_vmbits_readshort(state);
                    if(nn_value_isfalse(nn_vmbits_stackpeek(state, 0)))
                    {
                        state->vmstate.currentframe->inscode += offset;
                    }
                }
                break;
            case NEON_OP_LOOP:
                {
                    uint16_t offset;
                    offset = nn_vmbits_readshort(state);
                    state->vmstate.currentframe->inscode -= offset;
                }
                break;
            case NEON_OP_ECHO:
                {
                    NNValue val;
                    val = nn_vmbits_stackpeek(state, 0);
                    nn_printer_printvalue(state->stdoutprinter, val, state->isrepl, true);
                    if(!nn_value_isempty(val))
                    {
                        nn_printer_writestring(state->stdoutprinter, "\n");
                    }
                    nn_vmbits_stackpop(state);
                }
                break;
            case NEON_OP_STRINGIFY:
                {
                    NNValue peeked;
                    NNObjString* value;
                    peeked = nn_vmbits_stackpeek(state, 0);
                    if(!nn_value_isstring(peeked) && !nn_value_isnull(peeked))
                    {
                        value = nn_value_tostring(state, nn_vmbits_stackpop(state));
                        if(value->sbuf->length != 0)
                        {
                            nn_vmbits_stackpush(state, nn_value_fromobject(value));
                        }
                        else
                        {
                            nn_vmbits_stackpush(state, nn_value_makenull());
                        }
                    }
                }
                break;
            case NEON_OP_DUPONE:
                {
                    nn_vmbits_stackpush(state, nn_vmbits_stackpeek(state, 0));
                }
                break;
            case NEON_OP_POPONE:
                {
                    nn_vmbits_stackpop(state);
                }
                break;
            case NEON_OP_POPN:
                {
                    nn_vmbits_stackpopn(state, nn_vmbits_readshort(state));
                }
                break;
            case NEON_OP_UPVALUECLOSE:
                {
                    nn_vmutil_upvaluesclose(state, &state->vmstate.stackvalues[state->vmstate.stackidx - 1]);
                    nn_vmbits_stackpop(state);
                }
                break;
            case NEON_OP_GLOBALDEFINE:
                {
                    if(!nn_vmdo_globaldefine(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_GLOBALGET:
                {
                    if(!nn_vmdo_globalget(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_GLOBALSET:
                {
                    if(!nn_vmdo_globalset(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_LOCALGET:
                {
                    if(!nn_vmdo_localget(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_LOCALSET:
                {
                    if(!nn_vmdo_localset(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_FUNCARGGET:
                {
                    if(!nn_vmdo_funcargget(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_FUNCARGSET:
                {
                    if(!nn_vmdo_funcargset(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;

            case NEON_OP_PROPERTYGET:
                {
                    if(!nn_vmdo_propertyget(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_PROPERTYSET:
                {
                    if(!nn_vmdo_propertyset(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_PROPERTYGETSELF:
                {
                    if(!nn_vmdo_propertygetself(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_MAKECLOSURE:
                {
                    if(!nn_vmdo_makeclosure(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_UPVALUEGET:
                {
                    int index;
                    NNObjFuncClosure* closure;
                    index = nn_vmbits_readshort(state);
                    closure = state->vmstate.currentframe->closure;
                    if(index < closure->upvalcount)
                    {
                        nn_vmbits_stackpush(state, closure->upvalues[index]->location);
                    }
                    else
                    {
                        nn_vmbits_stackpush(state, nn_value_makeempty());
                    }
                }
                break;
            case NEON_OP_UPVALUESET:
                {
                    int index;
                    index = nn_vmbits_readshort(state);
                    if(nn_value_isempty(nn_vmbits_stackpeek(state, 0)))
                    {
                        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "empty cannot be assigned");
                        break;
                    }
                    state->vmstate.currentframe->closure->upvalues[index]->location = nn_vmbits_stackpeek(state, 0);
                }
                break;
            case NEON_OP_CALLFUNCTION:
                {
                    int argcount;
                    argcount = nn_vmbits_readbyte(state);
                    if(!nn_vm_callvalue(state, nn_vmbits_stackpeek(state, argcount), nn_value_makeempty(), argcount))
                    {
                        nn_vmmac_exitvm(state);
                    }
                    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                }
                break;
            case NEON_OP_CALLMETHOD:
                {
                    int argcount;
                    NNObjString* method;
                    method = nn_vmbits_readstring(state);
                    argcount = nn_vmbits_readbyte(state);
                    if(!nn_vmutil_invokemethod(state, method, argcount))
                    {
                        nn_vmmac_exitvm(state);
                    }
                    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                }
                break;
            case NEON_OP_CLASSGETTHIS:
                {
                    NNValue thisval;
                    thisval = nn_vmbits_stackpeek(state, 3);
                    nn_printer_writefmt(state->debugwriter, "CLASSGETTHIS: thisval=");
                    nn_printer_printvalue(state->debugwriter, thisval, true, false);
                    nn_printer_writefmt(state->debugwriter, "\n");
                    nn_vmbits_stackpush(state, thisval);
                }
                break;
            case NEON_OP_CLASSINVOKETHIS:
                {
                    int argcount;
                    NNObjString* method;
                    method = nn_vmbits_readstring(state);
                    argcount = nn_vmbits_readbyte(state);
                    if(!nn_vmutil_invokemethodself(state, method, argcount))
                    {
                        nn_vmmac_exitvm(state);
                    }
                    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                }
                break;
            case NEON_OP_MAKECLASS:
                {
                    bool haveval;
                    NNValue pushme;
                    NNObjString* name;
                    NNObjClass* klass;
                    NNProperty* field;
                    haveval = false;
                    name = nn_vmbits_readstring(state);
                    field = nn_table_getfieldbyostr(state->vmstate.currentframe->closure->scriptfunc->module->deftable, name);
                    if(field != NULL)
                    {
                        if(nn_value_isclass(field->value))
                        {
                            haveval = true;
                            pushme = field->value;
                        }
                    }
                    field = nn_table_getfieldbyostr(state->globals, name);
                    if(field != NULL)
                    {
                        if(nn_value_isclass(field->value))
                        {
                            haveval = true;
                            pushme = field->value;
                        }
                    }
                    if(!haveval)
                    {
                        klass = nn_object_makeclass(state, name);
                        pushme = nn_value_fromobject(klass);
                    }
                    nn_vmbits_stackpush(state, pushme);
                }
                break;
            case NEON_OP_MAKEMETHOD:
                {
                    NNObjString* name;
                    name = nn_vmbits_readstring(state);
                    nn_vmutil_definemethod(state, name);
                }
                break;
            case NEON_OP_CLASSPROPERTYDEFINE:
                {
                    int isstatic;
                    NNObjString* name;
                    name = nn_vmbits_readstring(state);
                    isstatic = nn_vmbits_readbyte(state);
                    nn_vmutil_defineproperty(state, name, isstatic == 1);
                }
                break;
            case NEON_OP_CLASSINHERIT:
                {
                    NNObjClass* superclass;
                    NNObjClass* subclass;
                    if(!nn_value_isclass(nn_vmbits_stackpeek(state, 1)))
                    {
                        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "cannot inherit from non-class object");
                        break;
                    }
                    superclass = nn_value_asclass(nn_vmbits_stackpeek(state, 1));
                    subclass = nn_value_asclass(nn_vmbits_stackpeek(state, 0));
                    nn_class_inheritfrom(subclass, superclass);
                    /* pop the subclass */
                    nn_vmbits_stackpop(state);
                }
                break;
            case NEON_OP_CLASSGETSUPER:
                {
                    NNObjClass* klass;
                    NNObjString* name;
                    name = nn_vmbits_readstring(state);
                    klass = nn_value_asclass(nn_vmbits_stackpeek(state, 0));
                    if(!nn_vmutil_bindmethod(state, klass->superclass, name))
                    {
                        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "class %s does not define a function %s", klass->name->sbuf->data, name->sbuf->data);
                    }
                }
                break;
            case NEON_OP_CLASSINVOKESUPER:
                {
                    int argcount;
                    NNObjClass* klass;
                    NNObjString* method;
                    method = nn_vmbits_readstring(state);
                    argcount = nn_vmbits_readbyte(state);
                    klass = nn_value_asclass(nn_vmbits_stackpop(state));
                    if(!nn_vmutil_invokemethodfromclass(state, klass, method, argcount))
                    {
                        nn_vmmac_exitvm(state);
                    }
                    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                }
                break;
            case NEON_OP_CLASSINVOKESUPERSELF:
                {
                    int argcount;
                    NNObjClass* klass;
                    argcount = nn_vmbits_readbyte(state);
                    klass = nn_value_asclass(nn_vmbits_stackpop(state));
                    if(!nn_vmutil_invokemethodfromclass(state, klass, state->constructorname, argcount))
                    {
                        nn_vmmac_exitvm(state);
                    }
                    state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                }
                break;
            case NEON_OP_MAKEARRAY:
                {
                    if(!nn_vmdo_makearray(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;

            case NEON_OP_MAKERANGE:
                {
                    double lower;
                    double upper;
                    NNValue vupper;
                    NNValue vlower;
                    vupper = nn_vmbits_stackpeek(state, 0);
                    vlower = nn_vmbits_stackpeek(state, 1);
                    if(!nn_value_isnumber(vupper) || !nn_value_isnumber(vlower))
                    {
                        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "invalid range boundaries");
                        break;
                    }
                    lower = nn_value_asnumber(vlower);
                    upper = nn_value_asnumber(vupper);
                    nn_vmbits_stackpopn(state, 2);
                    nn_vmbits_stackpush(state, nn_value_fromobject(nn_object_makerange(state, lower, upper)));
                }
                break;
            case NEON_OP_MAKEDICT:
                {
                    if(!nn_vmdo_makedict(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_INDEXGETRANGED:
                {
                    if(!nn_vmdo_getrangedindex(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_INDEXGET:
                {
                    if(!nn_vmdo_indexget(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_INDEXSET:
                {
                    if(!nn_vmdo_indexset(state))
                    {
                        nn_vmmac_exitvm(state);
                    }
                }
                break;
            case NEON_OP_IMPORTIMPORT:
                {
                    NNValue res;
                    NNObjString* name;
                    NNObjModule* mod;
                    name = nn_value_asstring(nn_vmbits_stackpeek(state, 0));
                    fprintf(stderr, "IMPORTIMPORT: name='%s'\n", name->sbuf->data);
                    mod = nn_import_loadmodulescript(state, state->topmodule, name);
                    fprintf(stderr, "IMPORTIMPORT: mod='%p'\n", (void*)mod);
                    if(mod == NULL)
                    {
                        res = nn_value_makenull();
                    }
                    else
                    {
                        res = nn_value_fromobject(mod);
                    }
                    nn_vmbits_stackpush(state, res);
                }
                break;
            case NEON_OP_TYPEOF:
                {
                    NNValue res;
                    NNValue thing;
                    const char* result;
                    thing = nn_vmbits_stackpop(state);
                    result = nn_value_typename(thing);
                    res = nn_value_fromobject(nn_string_copycstr(state, result));
                    nn_vmbits_stackpush(state, res);
                }
                break;
            case NEON_OP_ASSERT:
                {
                    NNValue message;
                    NNValue expression;
                    message = nn_vmbits_stackpop(state);
                    expression = nn_vmbits_stackpop(state);
                    if(nn_value_isfalse(expression))
                    {
                        if(!nn_value_isnull(message))
                        {
                            nn_exceptions_throwclass(state, state->exceptions.asserterror, nn_value_tostring(state, message)->sbuf->data);
                        }
                        else
                        {
                            nn_exceptions_throwclass(state, state->exceptions.asserterror, "assertion failed");
                        }
                    }
                }
                break;
            case NEON_OP_EXTHROW:
                {
                    bool isok;
                    NNValue peeked;
                    NNValue stacktrace;
                    NNObjInstance* instance;
                    peeked = nn_vmbits_stackpeek(state, 0);
                    isok = (
                        nn_value_isinstance(peeked) ||
                        nn_util_isinstanceof(nn_value_asinstance(peeked)->klass, state->exceptions.stdexception)
                    );
                    if(!isok)
                    {
                        nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "instance of Exception expected");
                        break;
                    }
                    stacktrace = nn_exceptions_getstacktrace(state);
                    instance = nn_value_asinstance(peeked);
                    nn_instance_defproperty(instance, "stacktrace", stacktrace);
                    if(nn_exceptions_propagate(state))
                    {
                        state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                        break;
                    }
                    nn_vmmac_exitvm(state);
                }
            case NEON_OP_EXTRY:
                {
                    bool haveclass;
                    uint16_t addr;
                    uint16_t finaddr;
                    NNValue value;
                    NNObjString* type;
                    NNObjClass* exclass;
                    haveclass = false;
                    exclass = NULL;
                    type = nn_vmbits_readstring(state);
                    addr = nn_vmbits_readshort(state);
                    finaddr = nn_vmbits_readshort(state);
                    if(addr != 0)
                    {
                        if(!nn_table_get(state->globals, nn_value_fromobject(type), &value))
                        {
                            if(nn_value_isclass(value))
                            {
                                haveclass = true;
                                exclass = nn_value_asclass(value);
                            }
                        }
                        if(!haveclass)
                        {
                            /*
                            if(!nn_table_get(state->vmstate.currentframe->closure->scriptfunc->module->deftable, nn_value_fromobject(type), &value) || !nn_value_isclass(value))
                            {
                                nn_vmmac_tryraise(state, NEON_STATUS_FAILRUNTIME, "object of type '%s' is not an exception", type->sbuf->data);
                                break;
                            }
                            */
                            exclass = state->exceptions.stdexception;
                        }
                        nn_exceptions_pushhandler(state, exclass, addr, finaddr);
                    }
                    else
                    {
                        nn_exceptions_pushhandler(state, NULL, addr, finaddr);
                    }
                }
                break;
            case NEON_OP_EXPOPTRY:
                {
                    state->vmstate.currentframe->handlercount--;
                }
                break;
            case NEON_OP_EXPUBLISHTRY:
                {
                    state->vmstate.currentframe->handlercount--;
                    if(nn_exceptions_propagate(state))
                    {
                        state->vmstate.currentframe = &state->vmstate.framevalues[state->vmstate.framecount - 1];
                        break;
                    }
                    nn_vmmac_exitvm(state);
                }
                break;
            case NEON_OP_SWITCH:
                {
                    NNValue expr;
                    NNValue value;
                    NNObjSwitch* sw;
                    sw = nn_value_asswitch(nn_vmbits_readconst(state));
                    expr = nn_vmbits_stackpeek(state, 0);
                    if(nn_table_get(sw->table, expr, &value))
                    {
                        state->vmstate.currentframe->inscode += (int)nn_value_asnumber(value);
                    }
                    else if(sw->defaultjump != -1)
                    {
                        state->vmstate.currentframe->inscode += sw->defaultjump;
                    }
                    else
                    {
                        state->vmstate.currentframe->inscode += sw->exitjump;
                    }
                    nn_vmbits_stackpop(state);
                }
                break;
            default:
                {
                }
                break;
        }

    }
}

int nn_nestcall_prepare(NNState* state, NNValue callable, NNValue mthobj, NNObjArray* callarr)
{
    int arity;
    NNObjFuncClosure* closure;
    (void)state;
    arity = 0;
    if(nn_value_isfuncclosure(callable))
    {
        closure = nn_value_asfuncclosure(callable);
        arity = closure->scriptfunc->arity;
    }
    else if(nn_value_isfuncscript(callable))
    {
        arity = nn_value_asfuncscript(callable)->arity;
    }
    else if(nn_value_isfuncnative(callable))
    {
        #if 0
            arity = nn_value_asfuncnative(callable);
        #endif
    }
    if(arity > 0)
    {
        nn_array_push(callarr, nn_value_makenull());
        if(arity > 1)
        {
            nn_array_push(callarr, nn_value_makenull());
            if(arity > 2)
            {
                nn_array_push(callarr, mthobj);
            }
        }
    }
    return arity;
}

/* helper function to access call outside the state file. */
bool nn_nestcall_callfunction(NNState* state, NNValue callable, NNValue thisval, NNObjArray* args, NNValue* dest)
{
    size_t i;
    int argc;
    size_t pidx;
    NNStatus status;
    pidx = state->vmstate.stackidx;
    /* set the closure before the args */
    nn_vm_stackpush(state, callable);
    argc = 0;
    if(args && (argc = args->varray->listcount))
    {
        for(i = 0; i < args->varray->listcount; i++)
        {
            nn_vm_stackpush(state, args->varray->listitems[i]);
        }
    }
    if(!nn_vm_callvaluewithobject(state, callable, thisval, argc))
    {
        fprintf(stderr, "nestcall: nn_vm_callvalue() failed\n");
        abort();
    }
    status = nn_vm_runvm(state, state->vmstate.framecount - 1, NULL);
    if(status != NEON_STATUS_OK)
    {
        fprintf(stderr, "nestcall: call to runvm failed\n");
        abort();
    }
    *dest = state->vmstate.stackvalues[state->vmstate.stackidx - 1];
    nn_vm_stackpopn(state, argc + 1);
    state->vmstate.stackidx = pidx;
    return true;
}

NNObjFuncClosure* nn_state_compilesource(NNState* state, NNObjModule* module, bool fromeval, const char* source)
{
    NNBlob blob;
    NNObjFuncScript* function;
    NNObjFuncClosure* closure;
    nn_blob_init(state, &blob);
    function = nn_astparser_compilesource(state, module, source, &blob, false, fromeval);
    if(function == NULL)
    {
        nn_blob_destroy(state, &blob);
        return NULL;
    }
    if(!fromeval)
    {
        nn_vm_stackpush(state, nn_value_fromobject(function));
    }
    else
    {
        function->name = nn_string_copycstr(state, "(evaledcode)");
    }
    closure = nn_object_makefuncclosure(state, function);
    if(!fromeval)
    {
        nn_vm_stackpop(state);
        nn_vm_stackpush(state, nn_value_fromobject(closure));
    }
    nn_blob_destroy(state, &blob);
    return closure;
}

NNStatus nn_state_execsource(NNState* state, NNObjModule* module, const char* source, NNValue* dest)
{
    NNStatus status;
    NNObjFuncClosure* closure;
    nn_module_setfilefield(state, module);
    closure = nn_state_compilesource(state, module, false, source);
    if(closure == NULL)
    {
        return NEON_STATUS_FAILCOMPILE;
    }
    if(state->conf.exitafterbytecode)
    {
        return NEON_STATUS_OK;
    }
    nn_vm_callclosure(state, closure, nn_value_makenull(), 0);
    status = nn_vm_runvm(state, 0, dest);
    return status;
}

NNValue nn_state_evalsource(NNState* state, const char* source)
{
    bool ok;
    int argc;
    NNValue callme;
    NNValue retval;
    NNObjFuncClosure* closure;
    NNObjArray* args;
    (void)argc;
    closure = nn_state_compilesource(state, state->topmodule, true, source);
    callme = nn_value_fromobject(closure);
    args = nn_array_make(state);
    argc = nn_nestcall_prepare(state, callme, nn_value_makenull(), args);
    ok = nn_nestcall_callfunction(state, callme, nn_value_makenull(), args, &retval);
    if(!ok)
    {
        nn_exceptions_throw(state, "eval() failed");
    }
    return retval;
}

char* nn_cli_getinput(const char* prompt)
{
    #if !defined(NEON_USE_LINENOISE)
        enum { kMaxLineSize = 1024 };
        size_t len;
        char* rt;
        char rawline[kMaxLineSize+1] = {0};
        fprintf(stdout, "%s", prompt);
        fflush(stdout);
        rt = fgets(rawline, kMaxLineSize, stdin);
        len = strlen(rt);
        rt[len - 1] = 0;
        return rt;
    #else
        return linenoise(prompt);
    #endif
}

void nn_cli_addhistoryline(const char* line)
{
    #if !defined(NEON_USE_LINENOISE)
        (void)line;
    #else
        linenoiseHistoryAdd(line);
    #endif
}

void nn_cli_freeline(char* line)
{
    #if !defined(NEON_USE_LINENOISE)
        (void)line;
    #else
        linenoiseFree(line);
    #endif
}

#if !defined(NEON_PLAT_ISWASM)
bool nn_cli_repl(NNState* state)
{
    size_t i;
    int linelength;
    int bracecount;
    int parencount;
    int bracketcount;
    int doublequotecount;
    int singlequotecount;
    bool continuerepl;
    char* line;
    StringBuffer* source;
    const char* cursor;
    NNValue dest;
    state->isrepl = true;
    continuerepl = true;
    printf("Type \".exit\" to quit or \".credits\" for more information\n");
    source = dyn_strbuf_makeempty(0);
    bracecount = 0;
    parencount = 0;
    bracketcount = 0;
    singlequotecount = 0;
    doublequotecount = 0;
    #if !defined(NEON_PLAT_ISWINDOWS)
        /* linenoiseSetEncodingFunctions(linenoiseUtf8PrevCharLen, linenoiseUtf8NextCharLen, linenoiseUtf8ReadCode); */
        linenoiseSetMultiLine(0);
        linenoiseHistoryAdd(".exit");
    #endif
    while(true)
    {
        if(!continuerepl)
        {
            bracecount = 0;
            parencount = 0;
            bracketcount = 0;
            singlequotecount = 0;
            doublequotecount = 0;
            dyn_strbuf_reset(source);
            continuerepl = true;
        }
        cursor = "%> ";
        if(bracecount > 0 || bracketcount > 0 || parencount > 0)
        {
            cursor = ".. ";
        }
        else if(singlequotecount == 1 || doublequotecount == 1)
        {
            cursor = "";
        }
        line = nn_cli_getinput(cursor);
        fprintf(stderr, "line = %s. isexit=%d\n", line, strcmp(line, ".exit"));
        if(line == NULL || strcmp(line, ".exit") == 0)
        {
            dyn_strbuf_destroy(source);
            return true;
        }
        linelength = (int)strlen(line);
        if(strcmp(line, ".credits") == 0)
        {
            printf("\n" NEON_INFO_COPYRIGHT "\n\n");
            dyn_strbuf_reset(source);
            continue;
        }
        nn_cli_addhistoryline(line);
        if(linelength > 0 && line[0] == '#')
        {
            continue;
        }
        /* find count of { and }, ( and ), [ and ], " and ' */
        for(i = 0; i < (size_t)linelength; i++)
        {
            if(line[i] == '{')
            {
                bracecount++;
            }
            if(line[i] == '(')
            {
                parencount++;
            }
            if(line[i] == '[')
            {
                bracketcount++;
            }
            if(line[i] == '\'' && doublequotecount == 0)
            {
                if(singlequotecount == 0)
                {
                    singlequotecount++;
                }
                else
                {
                    singlequotecount--;
                }
            }
            if(line[i] == '"' && singlequotecount == 0)
            {
                if(doublequotecount == 0)
                {
                    doublequotecount++;
                }
                else
                {
                    doublequotecount--;
                }
            }
            if(line[i] == '\\' && (singlequotecount > 0 || doublequotecount > 0))
            {
                i++;
            }
            if(line[i] == '}' && bracecount > 0)
            {
                bracecount--;
            }
            if(line[i] == ')' && parencount > 0)
            {
                parencount--;
            }
            if(line[i] == ']' && bracketcount > 0)
            {
                bracketcount--;
            }
        }
        dyn_strbuf_appendstr(source, line);
        if(linelength > 0)
        {
            dyn_strbuf_appendstr(source, "\n");
        }
        nn_cli_freeline(line);
        if(bracketcount == 0 && parencount == 0 && bracecount == 0 && singlequotecount == 0 && doublequotecount == 0)
        {
            nn_state_execsource(state, state->topmodule, source->data, &dest);
            fflush(stdout);
            continuerepl = false;
        }
    }
    return true;
}
#endif

bool nn_cli_runfile(NNState* state, const char* file)
{
    size_t fsz;
    char* rp;
    char* source;
    const char* oldfile;
    NNStatus result;
    source = nn_util_readfile(state, file, &fsz);
    if(source == NULL)
    {
        oldfile = file;
        source = nn_util_readfile(state, file, &fsz);
        if(source == NULL)
        {
            fprintf(stderr, "failed to read from '%s': %s\n", oldfile, strerror(errno));
            return false;
        }
    }
    state->rootphysfile = (char*)file;
    rp = osfn_realpath(file, NULL);
    state->topmodule->physicalpath = nn_string_copycstr(state, rp);
    nn_util_memfree(state, rp);
    result = nn_state_execsource(state, state->topmodule, source, NULL);
    nn_util_memfree(state, source);
    fflush(stdout);
    if(result == NEON_STATUS_FAILCOMPILE)
    {
        return false;
    }
    if(result == NEON_STATUS_FAILRUNTIME)
    {
        return false;
    }
    return true;
}

bool nn_cli_runcode(NNState* state, char* source)
{
    NNStatus result;
    state->rootphysfile = NULL;
    result = nn_state_execsource(state, state->topmodule, source, NULL);
    fflush(stdout);
    if(result == NEON_STATUS_FAILCOMPILE)
    {
        return false;
    }
    if(result == NEON_STATUS_FAILRUNTIME)
    {
        return false;
    }
    return true;
}

#if defined(NEON_PLAT_ISWASM)
int __multi3(int a, int b)
{
    return a*b;
}
#endif

int nn_util_findfirstpos(const char* str, size_t len, int ch)
{
    size_t i;
    for(i=0; i<len; i++)
    {
        if(str[i] == ch)
        {
            return i;
        }
    }
    return -1;
}

void nn_cli_parseenv(NNState* state, char** envp)
{
    enum { kMaxKeyLen = 40 };
    size_t i;
    int len;
    int pos;
    char* raw;
    char* valbuf;
    char keybuf[kMaxKeyLen];
    NNObjString* oskey;
    NNObjString* osval;
    for(i=0; envp[i] != NULL; i++)
    {
        raw = envp[i];
        len = strlen(raw);
        pos = nn_util_findfirstpos(raw, len, '=');
        if(pos == -1)
        {
            fprintf(stderr, "malformed environment string '%s'\n", raw);
        }
        else
        {
            memset(keybuf, 0, kMaxKeyLen);
            memcpy(keybuf, raw, pos);
            valbuf = &raw[pos+1];
            oskey = nn_string_copycstr(state, keybuf);
            osval = nn_string_copycstr(state, valbuf);
            nn_dict_setentry(state->envdict, nn_value_fromobject(oskey), nn_value_fromobject(osval));
        }
    }
}

void nn_cli_printtypesizes()
{
    #define ptyp(t) \
        { \
            fprintf(stdout, "%d\t%s\n", (int)sizeof(t), #t); \
            fflush(stdout); \
        }
    ptyp(NNPrinter);
    ptyp(NNValue);
    ptyp(NNObject);
    ptyp(NNPropGetSet);
    ptyp(NNProperty);
    ptyp(NNValArray);
    ptyp(NNBlob);
    ptyp(NNHashEntry);
    ptyp(NNHashTable);
    ptyp(NNObjString);
    ptyp(NNObjUpvalue);
    ptyp(NNObjModule);
    ptyp(NNObjFuncScript);
    ptyp(NNObjFuncClosure);
    ptyp(NNObjClass);
    ptyp(NNObjInstance);
    ptyp(NNObjFuncBound);
    ptyp(NNObjFuncNative);
    ptyp(NNObjArray);
    ptyp(NNObjRange);
    ptyp(NNObjDict);
    ptyp(NNObjFile);
    ptyp(NNObjSwitch);
    ptyp(NNObjUserdata);
    ptyp(NNExceptionFrame);
    ptyp(NNCallFrame);
    ptyp(NNState);
    ptyp(NNAstToken);
    ptyp(NNAstLexer);
    ptyp(NNAstLocal);
    ptyp(NNAstUpvalue);
    ptyp(NNAstFuncCompiler);
    ptyp(NNAstClassCompiler);
    ptyp(NNAstParser);
    ptyp(NNAstRule);
    ptyp(NNRegFunc);
    ptyp(NNRegField);
    ptyp(NNRegClass);
    ptyp(NNRegModule);
    ptyp(NNInstruction)
    #undef ptyp
}


void optprs_fprintmaybearg(FILE* out, const char* begin, const char* flagname, size_t flaglen, bool needval, bool maybeval, const char* delim)
{
    fprintf(out, "%s%.*s", begin, (int)flaglen, flagname);
    if(needval)
    {
        if(maybeval)
        {
            fprintf(out, "[");
        }
        if(delim != NULL)
        {
            fprintf(out, "%s", delim);
        }
        fprintf(out, "<val>");
        if(maybeval)
        {
            fprintf(out, "]");
        }
    }
}

void optprs_fprintusage(FILE* out, optlongflags_t* flags)
{
    size_t i;
    char ch;
    bool needval;
    bool maybeval;
    bool hadshort;
    optlongflags_t* flag;
    for(i=0; flags[i].longname != NULL; i++)
    {
        flag = &flags[i];
        hadshort = false;
        needval = (flag->argtype > OPTPARSE_NONE);
        maybeval = (flag->argtype == OPTPARSE_OPTIONAL);
        if(flag->shortname > 0)
        {
            hadshort = true;
            ch = flag->shortname;
            fprintf(out, "    ");
            optprs_fprintmaybearg(out, "-", &ch, 1, needval, maybeval, NULL);
        }
        if(flag->longname != NULL)
        {
            if(hadshort)
            {
                fprintf(out, ", ");
            }
            else
            {
                fprintf(out, "    ");
            }
            optprs_fprintmaybearg(out, "--", flag->longname, strlen(flag->longname), needval, maybeval, "=");
        }
        if(flag->helptext != NULL)
        {
            fprintf(out, "  -  %s", flag->helptext);
        }
        fprintf(out, "\n");
    }
}

void nn_cli_showusage(char* argv[], optlongflags_t* flags, bool fail)
{
    FILE* out;
    out = fail ? stderr : stdout;
    fprintf(out, "Usage: %s [<options>] [<filename> | -e <code>]\n", argv[0]);
    optprs_fprintusage(out, flags);
}

int main(int argc, char* argv[], char** envp)
{
    int i;
    int co;
    int opt;
    int nargc;
    int longindex;
    int nextgcstart;
    bool ok;
    bool wasusage;
    bool quitafterinit;
    char *arg;
    char* source;
    const char* filename;
    char* nargv[128];
    optcontext_t options;
    NNState* state;
    static optlongflags_t longopts[] =
    {
        {"help", 'h', OPTPARSE_NONE, "this help"},
        {"strict", 's', OPTPARSE_NONE, "enable strict mode, such as requiring explicit var declarations"},
        {"warn", 'w', OPTPARSE_NONE, "enable warnings"},
        {"debug", 'd', OPTPARSE_NONE, "enable debugging: print instructions and stack values during execution"},
        {"exitaftercompile", 'x', OPTPARSE_NONE, "when using '-d', quit after printing compiled function(s)"},
        {"eval", 'e', OPTPARSE_REQUIRED, "evaluate a single line of code"},
        {"quit", 'q', OPTPARSE_NONE, "initiate, then immediately destroy the interpreter state"},
        {"types", 't', OPTPARSE_NONE, "print sizeof() of types"},
        {"apidebug", 'a', OPTPARSE_NONE, "print calls to API (very verbose, very slow)"},
        {"astdebug", 'A', OPTPARSE_NONE, "print calls to the parser (very verbose, very slow)"},
        {"gcstart", 'g', OPTPARSE_REQUIRED, "set minimum bytes at which the GC should kick in. 0 disables GC"},
        {0, 0, (optargtype_t)0, NULL}
    };
    #if defined(NEON_PLAT_ISWINDOWS)
        _setmode(fileno(stdin), _O_BINARY);
        _setmode(fileno(stdout), _O_BINARY);
        _setmode(fileno(stderr), _O_BINARY);
    #endif
    ok = true;
    wasusage = false;
    quitafterinit = false;
    source = NULL;
    nextgcstart = NEON_CFG_DEFAULTGCSTART;
    state = nn_state_make();
    nargc = 0;
    optprs_init(&options, argc, argv);
    options.permute = 0;
    while ((opt = optprs_nextlongflag(&options, longopts, &longindex)) != -1)
    {
        co = longopts[longindex].shortname;
        if(opt == '?')
        {
            printf("%s: %s\n", argv[0], options.errmsg);
        }
        else if(co == 'g')
        {
            nextgcstart = atol(options.optarg);
        }
        else if(co == 't')
        {
            nn_cli_printtypesizes();
            return 0;
        }
        else if(co == 'h')
        {
            nn_cli_showusage(argv, longopts, false);
            wasusage = true;
        }
        else if(co == 'd' || co == 'j')
        {
            state->conf.dumpbytecode = true;
            state->conf.shoulddumpstack = true;        
        }
        else if(co == 'x')
        {
            state->conf.exitafterbytecode = true;
        }
        else if(co == 'a')
        {
            state->conf.enableapidebug = true;
        }
        else if(co == 'A')
        {
            state->conf.enableastdebug = true;
        }
        else if(co == 's')
        {
            state->conf.enablestrictmode = true;            
        }
        else if(co == 'e')
        {
            source = options.optarg;
        }
        else if(co == 'w')
        {
            state->conf.enablewarnings = true;
        }
        else if(co == 'q')
        {
            quitafterinit = true;
        }
    }
    if(wasusage || quitafterinit)
    {
        goto cleanup;
    }
    nn_cli_parseenv(state, envp);
    while(true)
    {
        arg = optprs_nextpositional(&options);
        if(arg == NULL)
        {
            break;
        }
        nargv[nargc] = arg;
        nargc++;
    }
    {
        NNObjString* os;
        state->cliargv = nn_object_makearray(state);
        for(i=0; i<nargc; i++)
        {
            os = nn_string_copycstr(state, nargv[i]);
            nn_array_push(state->cliargv, nn_value_fromobject(os));

        }
        nn_table_setcstr(state->globals, "ARGV", nn_value_fromobject(state->cliargv));
    }
    state->gcstate.nextgc = nextgcstart;
    nn_import_loadbuiltinmodules(state);
    if(source != NULL)
    {
        ok = nn_cli_runcode(state, source);
    }
    else if(nargc > 0)
    {
        filename = nn_value_asstring(state->cliargv->varray->listitems[0])->sbuf->data;
        fprintf(stderr, "nargv[0]=%s\n", filename);
        ok = nn_cli_runfile(state,  filename);
    }
    else
    {
        ok = nn_cli_repl(state);
    }
    cleanup:
    nn_state_destroy(state);
    if(ok)
    {
        return 0;
    }
    return 1;
}


