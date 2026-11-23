

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.01.0628 */
/* at Mon Jan 18 19:14:07 2038
 */
/* Compiler settings for D:\UDM\native\Udm.Automation.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "Udm.Automation_h.h"

#define TYPE_FORMAT_STRING_SIZE   999                               
#define PROC_FORMAT_STRING_SIZE   229                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   2            

typedef struct _Udm2EAutomation_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } Udm2EAutomation_MIDL_TYPE_FORMAT_STRING;

typedef struct _Udm2EAutomation_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } Udm2EAutomation_MIDL_PROC_FORMAT_STRING;

typedef struct _Udm2EAutomation_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } Udm2EAutomation_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax_2_0 = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};

#if defined(_CONTROL_FLOW_GUARD_XFG)
#define XFG_TRAMPOLINES(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree(pFlags, (ObjectType *)pObject);\
}
#define XFG_TRAMPOLINES64(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize64_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize64(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree64_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree64(pFlags, (ObjectType *)pObject);\
}
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)\
static void* ObjectType ## _bind_XFG(HandleType pObject)\
{\
return ObjectType ## _bind((ObjectType) pObject);\
}\
static void ObjectType ## _unbind_XFG(HandleType pObject, handle_t ServerHandle)\
{\
ObjectType ## _unbind((ObjectType) pObject, ServerHandle);\
}
#define XFG_TRAMPOLINE_FPTR(Function) Function ## _XFG
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol ## _XFG
#else
#define XFG_TRAMPOLINES(ObjectType)
#define XFG_TRAMPOLINES64(ObjectType)
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)
#define XFG_TRAMPOLINE_FPTR(Function) Function
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol
#endif


extern const Udm2EAutomation_MIDL_TYPE_FORMAT_STRING Udm2EAutomation__MIDL_TypeFormatString;
extern const Udm2EAutomation_MIDL_PROC_FORMAT_STRING Udm2EAutomation__MIDL_ProcFormatString;
extern const Udm2EAutomation_MIDL_EXPR_FORMAT_STRING Udm2EAutomation__MIDL_ExprFormatString;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IUdmLinkTransmitter_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IUdmLinkTransmitter_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const Udm2EAutomation_MIDL_PROC_FORMAT_STRING Udm2EAutomation__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure SendLinkToUDM */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x7 ),	/* 7 */
/*  8 */	NdrFcShort( 0x58 ),	/* X64 Stack size/offset = 88 */
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0xa,		/* 10 */
/* 16 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x1 ),	/* 1 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter url */

/* 26 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter referer */

/* 32 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter cookies */

/* 38 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 40 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 42 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter data */

/* 44 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 46 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 48 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter user */

/* 50 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 52 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 54 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter password */

/* 56 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 58 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 60 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter localPath */

/* 62 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 64 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 66 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter localFileName */

/* 68 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 70 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 72 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter flags */

/* 74 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 76 */	NdrFcShort( 0x48 ),	/* X64 Stack size/offset = 72 */
/* 78 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 80 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 82 */	NdrFcShort( 0x50 ),	/* X64 Stack size/offset = 80 */
/* 84 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SendLinkToUDM2 */

/* 86 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 88 */	NdrFcLong( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x8 ),	/* 8 */
/* 94 */	NdrFcShort( 0x68 ),	/* X64 Stack size/offset = 104 */
/* 96 */	NdrFcShort( 0x8 ),	/* 8 */
/* 98 */	NdrFcShort( 0x8 ),	/* 8 */
/* 100 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0xc,		/* 12 */
/* 102 */	0xa,		/* 10 */
			0x85,		/* Ext Flags:  new corr desc, srv corr check, has big byval param */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x1 ),	/* 1 */
/* 108 */	NdrFcShort( 0x0 ),	/* 0 */
/* 110 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter url */

/* 112 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 114 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 116 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter referer */

/* 118 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 120 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 122 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter cookies */

/* 124 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 126 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 128 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter data */

/* 130 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 132 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 134 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter user */

/* 136 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 138 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 140 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter password */

/* 142 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 144 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 146 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter localPath */

/* 148 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 150 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 152 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter localFileName */

/* 154 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 156 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 158 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter flags */

/* 160 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 162 */	NdrFcShort( 0x48 ),	/* X64 Stack size/offset = 72 */
/* 164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter userAgent */

/* 166 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 168 */	NdrFcShort( 0x50 ),	/* X64 Stack size/offset = 80 */
/* 170 */	NdrFcShort( 0x3dc ),	/* Type Offset=988 */

	/* Parameter reserved */

/* 172 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 174 */	NdrFcShort( 0x58 ),	/* X64 Stack size/offset = 88 */
/* 176 */	NdrFcShort( 0x3dc ),	/* Type Offset=988 */

	/* Return value */

/* 178 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 180 */	NdrFcShort( 0x60 ),	/* X64 Stack size/offset = 96 */
/* 182 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SendLinksArray */

/* 184 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 186 */	NdrFcLong( 0x0 ),	/* 0 */
/* 190 */	NdrFcShort( 0x9 ),	/* 9 */
/* 192 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 198 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 200 */	0xa,		/* 10 */
			0x85,		/* Ext Flags:  new corr desc, srv corr check, has big byval param */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 204 */	NdrFcShort( 0x1 ),	/* 1 */
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter location */

/* 210 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 212 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 214 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter links */

/* 216 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 218 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 220 */	NdrFcShort( 0x3dc ),	/* Type Offset=988 */

	/* Return value */

/* 222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 224 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const Udm2EAutomation_MIDL_TYPE_FORMAT_STRING Udm2EAutomation__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0xe ),	/* Offset= 14 (18) */
/*  6 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/*  8 */	NdrFcShort( 0x2 ),	/* 2 */
/* 10 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 12 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 14 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 16 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 18 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 20 */	NdrFcShort( 0x8 ),	/* 8 */
/* 22 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (6) */
/* 24 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 26 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 28 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 30 */	NdrFcShort( 0x0 ),	/* 0 */
/* 32 */	NdrFcShort( 0x8 ),	/* 8 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0xffde ),	/* Offset= -34 (2) */
/* 38 */	
			0x11, 0x0,	/* FC_RP */
/* 40 */	NdrFcShort( 0x3b4 ),	/* Offset= 948 (988) */
/* 42 */	
			0x12, 0x0,	/* FC_UP */
/* 44 */	NdrFcShort( 0x39c ),	/* Offset= 924 (968) */
/* 46 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 48 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 50 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 52 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 54 */	NdrFcShort( 0x2 ),	/* Offset= 2 (56) */
/* 56 */	NdrFcShort( 0x10 ),	/* 16 */
/* 58 */	NdrFcShort( 0x2f ),	/* 47 */
/* 60 */	NdrFcLong( 0x14 ),	/* 20 */
/* 64 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 66 */	NdrFcLong( 0x3 ),	/* 3 */
/* 70 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 72 */	NdrFcLong( 0x11 ),	/* 17 */
/* 76 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 78 */	NdrFcLong( 0x2 ),	/* 2 */
/* 82 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 84 */	NdrFcLong( 0x4 ),	/* 4 */
/* 88 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 90 */	NdrFcLong( 0x5 ),	/* 5 */
/* 94 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 96 */	NdrFcLong( 0xb ),	/* 11 */
/* 100 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 102 */	NdrFcLong( 0xa ),	/* 10 */
/* 106 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 108 */	NdrFcLong( 0x6 ),	/* 6 */
/* 112 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (344) */
/* 114 */	NdrFcLong( 0x7 ),	/* 7 */
/* 118 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 120 */	NdrFcLong( 0x8 ),	/* 8 */
/* 124 */	NdrFcShort( 0xff86 ),	/* Offset= -122 (2) */
/* 126 */	NdrFcLong( 0xd ),	/* 13 */
/* 130 */	NdrFcShort( 0xdc ),	/* Offset= 220 (350) */
/* 132 */	NdrFcLong( 0x9 ),	/* 9 */
/* 136 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (368) */
/* 138 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 142 */	NdrFcShort( 0xf4 ),	/* Offset= 244 (386) */
/* 144 */	NdrFcLong( 0x24 ),	/* 36 */
/* 148 */	NdrFcShort( 0x2ea ),	/* Offset= 746 (894) */
/* 150 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 154 */	NdrFcShort( 0x2e4 ),	/* Offset= 740 (894) */
/* 156 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 160 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (898) */
/* 162 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 166 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (902) */
/* 168 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 172 */	NdrFcShort( 0x2de ),	/* Offset= 734 (906) */
/* 174 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 178 */	NdrFcShort( 0x2dc ),	/* Offset= 732 (910) */
/* 180 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 184 */	NdrFcShort( 0x2da ),	/* Offset= 730 (914) */
/* 186 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 190 */	NdrFcShort( 0x2d8 ),	/* Offset= 728 (918) */
/* 192 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 196 */	NdrFcShort( 0x2c2 ),	/* Offset= 706 (902) */
/* 198 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 202 */	NdrFcShort( 0x2c0 ),	/* Offset= 704 (906) */
/* 204 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 208 */	NdrFcShort( 0x2ca ),	/* Offset= 714 (922) */
/* 210 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 214 */	NdrFcShort( 0x2c0 ),	/* Offset= 704 (918) */
/* 216 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 220 */	NdrFcShort( 0x2c2 ),	/* Offset= 706 (926) */
/* 222 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 226 */	NdrFcShort( 0x2c0 ),	/* Offset= 704 (930) */
/* 228 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 232 */	NdrFcShort( 0x2be ),	/* Offset= 702 (934) */
/* 234 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 238 */	NdrFcShort( 0x2bc ),	/* Offset= 700 (938) */
/* 240 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 244 */	NdrFcShort( 0x2ba ),	/* Offset= 698 (942) */
/* 246 */	NdrFcLong( 0x10 ),	/* 16 */
/* 250 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 252 */	NdrFcLong( 0x12 ),	/* 18 */
/* 256 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 258 */	NdrFcLong( 0x13 ),	/* 19 */
/* 262 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 264 */	NdrFcLong( 0x15 ),	/* 21 */
/* 268 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 270 */	NdrFcLong( 0x16 ),	/* 22 */
/* 274 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 276 */	NdrFcLong( 0x17 ),	/* 23 */
/* 280 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 282 */	NdrFcLong( 0xe ),	/* 14 */
/* 286 */	NdrFcShort( 0x298 ),	/* Offset= 664 (950) */
/* 288 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 292 */	NdrFcShort( 0x29c ),	/* Offset= 668 (960) */
/* 294 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 298 */	NdrFcShort( 0x29a ),	/* Offset= 666 (964) */
/* 300 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 304 */	NdrFcShort( 0x256 ),	/* Offset= 598 (902) */
/* 306 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 310 */	NdrFcShort( 0x254 ),	/* Offset= 596 (906) */
/* 312 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 316 */	NdrFcShort( 0x252 ),	/* Offset= 594 (910) */
/* 318 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 322 */	NdrFcShort( 0x248 ),	/* Offset= 584 (906) */
/* 324 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 328 */	NdrFcShort( 0x242 ),	/* Offset= 578 (906) */
/* 330 */	NdrFcLong( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* Offset= 0 (334) */
/* 336 */	NdrFcLong( 0x1 ),	/* 1 */
/* 340 */	NdrFcShort( 0x0 ),	/* Offset= 0 (340) */
/* 342 */	NdrFcShort( 0xffff ),	/* Offset= -1 (341) */
/* 344 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 346 */	NdrFcShort( 0x8 ),	/* 8 */
/* 348 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 350 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 352 */	NdrFcLong( 0x0 ),	/* 0 */
/* 356 */	NdrFcShort( 0x0 ),	/* 0 */
/* 358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 360 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 362 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 364 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 366 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 368 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 370 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 376 */	NdrFcShort( 0x0 ),	/* 0 */
/* 378 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 380 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 382 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 384 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 386 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 388 */	NdrFcShort( 0x2 ),	/* Offset= 2 (390) */
/* 390 */	
			0x12, 0x0,	/* FC_UP */
/* 392 */	NdrFcShort( 0x1e4 ),	/* Offset= 484 (876) */
/* 394 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x89,		/* 137 */
/* 396 */	NdrFcShort( 0x20 ),	/* 32 */
/* 398 */	NdrFcShort( 0xa ),	/* 10 */
/* 400 */	NdrFcLong( 0x8 ),	/* 8 */
/* 404 */	NdrFcShort( 0x50 ),	/* Offset= 80 (484) */
/* 406 */	NdrFcLong( 0xd ),	/* 13 */
/* 410 */	NdrFcShort( 0x70 ),	/* Offset= 112 (522) */
/* 412 */	NdrFcLong( 0x9 ),	/* 9 */
/* 416 */	NdrFcShort( 0x90 ),	/* Offset= 144 (560) */
/* 418 */	NdrFcLong( 0xc ),	/* 12 */
/* 422 */	NdrFcShort( 0xb0 ),	/* Offset= 176 (598) */
/* 424 */	NdrFcLong( 0x24 ),	/* 36 */
/* 428 */	NdrFcShort( 0x102 ),	/* Offset= 258 (686) */
/* 430 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 434 */	NdrFcShort( 0x11e ),	/* Offset= 286 (720) */
/* 436 */	NdrFcLong( 0x10 ),	/* 16 */
/* 440 */	NdrFcShort( 0x138 ),	/* Offset= 312 (752) */
/* 442 */	NdrFcLong( 0x2 ),	/* 2 */
/* 446 */	NdrFcShort( 0x14e ),	/* Offset= 334 (780) */
/* 448 */	NdrFcLong( 0x3 ),	/* 3 */
/* 452 */	NdrFcShort( 0x164 ),	/* Offset= 356 (808) */
/* 454 */	NdrFcLong( 0x14 ),	/* 20 */
/* 458 */	NdrFcShort( 0x17a ),	/* Offset= 378 (836) */
/* 460 */	NdrFcShort( 0xffff ),	/* Offset= -1 (459) */
/* 462 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 464 */	NdrFcShort( 0x0 ),	/* 0 */
/* 466 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 470 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 472 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 476 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 478 */	
			0x12, 0x0,	/* FC_UP */
/* 480 */	NdrFcShort( 0xfe32 ),	/* Offset= -462 (18) */
/* 482 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 484 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 486 */	NdrFcShort( 0x10 ),	/* 16 */
/* 488 */	NdrFcShort( 0x0 ),	/* 0 */
/* 490 */	NdrFcShort( 0x6 ),	/* Offset= 6 (496) */
/* 492 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 494 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 496 */	
			0x11, 0x0,	/* FC_RP */
/* 498 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (462) */
/* 500 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 502 */	NdrFcShort( 0x0 ),	/* 0 */
/* 504 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 506 */	NdrFcShort( 0x0 ),	/* 0 */
/* 508 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 510 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 514 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 516 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 518 */	NdrFcShort( 0xff58 ),	/* Offset= -168 (350) */
/* 520 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 522 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 524 */	NdrFcShort( 0x10 ),	/* 16 */
/* 526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 528 */	NdrFcShort( 0x6 ),	/* Offset= 6 (534) */
/* 530 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 532 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 534 */	
			0x11, 0x0,	/* FC_RP */
/* 536 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (500) */
/* 538 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 542 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 546 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 548 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 552 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 554 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 556 */	NdrFcShort( 0xff44 ),	/* Offset= -188 (368) */
/* 558 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 560 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 562 */	NdrFcShort( 0x10 ),	/* 16 */
/* 564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x6 ),	/* Offset= 6 (572) */
/* 568 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 570 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 572 */	
			0x11, 0x0,	/* FC_RP */
/* 574 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (538) */
/* 576 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 578 */	NdrFcShort( 0x0 ),	/* 0 */
/* 580 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 584 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 586 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 590 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 592 */	
			0x12, 0x0,	/* FC_UP */
/* 594 */	NdrFcShort( 0x176 ),	/* Offset= 374 (968) */
/* 596 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 598 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 600 */	NdrFcShort( 0x10 ),	/* 16 */
/* 602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 604 */	NdrFcShort( 0x6 ),	/* Offset= 6 (610) */
/* 606 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 608 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 610 */	
			0x11, 0x0,	/* FC_RP */
/* 612 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (576) */
/* 614 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 616 */	NdrFcLong( 0x2f ),	/* 47 */
/* 620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 622 */	NdrFcShort( 0x0 ),	/* 0 */
/* 624 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 626 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 628 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 630 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 632 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 634 */	NdrFcShort( 0x1 ),	/* 1 */
/* 636 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 638 */	NdrFcShort( 0x4 ),	/* 4 */
/* 640 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 642 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 644 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 646 */	NdrFcShort( 0x18 ),	/* 24 */
/* 648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 650 */	NdrFcShort( 0xa ),	/* Offset= 10 (660) */
/* 652 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 654 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 656 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (614) */
/* 658 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 660 */	
			0x12, 0x0,	/* FC_UP */
/* 662 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (632) */
/* 664 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 668 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 672 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 674 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 678 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 680 */	
			0x12, 0x0,	/* FC_UP */
/* 682 */	NdrFcShort( 0xffda ),	/* Offset= -38 (644) */
/* 684 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 686 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 688 */	NdrFcShort( 0x10 ),	/* 16 */
/* 690 */	NdrFcShort( 0x0 ),	/* 0 */
/* 692 */	NdrFcShort( 0x6 ),	/* Offset= 6 (698) */
/* 694 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 696 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 698 */	
			0x11, 0x0,	/* FC_RP */
/* 700 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (664) */
/* 702 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 704 */	NdrFcShort( 0x8 ),	/* 8 */
/* 706 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 708 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 710 */	NdrFcShort( 0x10 ),	/* 16 */
/* 712 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 714 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 716 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (702) */
			0x5b,		/* FC_END */
/* 720 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 722 */	NdrFcShort( 0x20 ),	/* 32 */
/* 724 */	NdrFcShort( 0x0 ),	/* 0 */
/* 726 */	NdrFcShort( 0xa ),	/* Offset= 10 (736) */
/* 728 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 730 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 732 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (708) */
			0x5b,		/* FC_END */
/* 736 */	
			0x11, 0x0,	/* FC_RP */
/* 738 */	NdrFcShort( 0xff12 ),	/* Offset= -238 (500) */
/* 740 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 742 */	NdrFcShort( 0x1 ),	/* 1 */
/* 744 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 748 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 750 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 752 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 754 */	NdrFcShort( 0x10 ),	/* 16 */
/* 756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 758 */	NdrFcShort( 0x6 ),	/* Offset= 6 (764) */
/* 760 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 762 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 764 */	
			0x12, 0x0,	/* FC_UP */
/* 766 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (740) */
/* 768 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 770 */	NdrFcShort( 0x2 ),	/* 2 */
/* 772 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 774 */	NdrFcShort( 0x0 ),	/* 0 */
/* 776 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 778 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 780 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 782 */	NdrFcShort( 0x10 ),	/* 16 */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 786 */	NdrFcShort( 0x6 ),	/* Offset= 6 (792) */
/* 788 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 790 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 792 */	
			0x12, 0x0,	/* FC_UP */
/* 794 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (768) */
/* 796 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 798 */	NdrFcShort( 0x4 ),	/* 4 */
/* 800 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 802 */	NdrFcShort( 0x0 ),	/* 0 */
/* 804 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 806 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 808 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 810 */	NdrFcShort( 0x10 ),	/* 16 */
/* 812 */	NdrFcShort( 0x0 ),	/* 0 */
/* 814 */	NdrFcShort( 0x6 ),	/* Offset= 6 (820) */
/* 816 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 818 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 820 */	
			0x12, 0x0,	/* FC_UP */
/* 822 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (796) */
/* 824 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 826 */	NdrFcShort( 0x8 ),	/* 8 */
/* 828 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 832 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 834 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 836 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 838 */	NdrFcShort( 0x10 ),	/* 16 */
/* 840 */	NdrFcShort( 0x0 ),	/* 0 */
/* 842 */	NdrFcShort( 0x6 ),	/* Offset= 6 (848) */
/* 844 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 846 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 848 */	
			0x12, 0x0,	/* FC_UP */
/* 850 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (824) */
/* 852 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 854 */	NdrFcShort( 0x8 ),	/* 8 */
/* 856 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 858 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 860 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 862 */	NdrFcShort( 0x8 ),	/* 8 */
/* 864 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 866 */	NdrFcShort( 0xffc8 ),	/* -56 */
/* 868 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 870 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 872 */	NdrFcShort( 0xffec ),	/* Offset= -20 (852) */
/* 874 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 876 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 878 */	NdrFcShort( 0x38 ),	/* 56 */
/* 880 */	NdrFcShort( 0xffec ),	/* Offset= -20 (860) */
/* 882 */	NdrFcShort( 0x0 ),	/* Offset= 0 (882) */
/* 884 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 886 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 888 */	0x40,		/* FC_STRUCTPAD4 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 890 */	0x0,		/* 0 */
			NdrFcShort( 0xfe0f ),	/* Offset= -497 (394) */
			0x5b,		/* FC_END */
/* 894 */	
			0x12, 0x0,	/* FC_UP */
/* 896 */	NdrFcShort( 0xff04 ),	/* Offset= -252 (644) */
/* 898 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 900 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 902 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 904 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 906 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 908 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 910 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 912 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 914 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 916 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 918 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 920 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 922 */	
			0x12, 0x0,	/* FC_UP */
/* 924 */	NdrFcShort( 0xfdbc ),	/* Offset= -580 (344) */
/* 926 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 928 */	NdrFcShort( 0xfc62 ),	/* Offset= -926 (2) */
/* 930 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 932 */	NdrFcShort( 0xfdba ),	/* Offset= -582 (350) */
/* 934 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 936 */	NdrFcShort( 0xfdc8 ),	/* Offset= -568 (368) */
/* 938 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 940 */	NdrFcShort( 0xfdd6 ),	/* Offset= -554 (386) */
/* 942 */	
			0x12, 0x10,	/* FC_UP [pointer_deref] */
/* 944 */	NdrFcShort( 0x2 ),	/* Offset= 2 (946) */
/* 946 */	
			0x12, 0x0,	/* FC_UP */
/* 948 */	NdrFcShort( 0x14 ),	/* Offset= 20 (968) */
/* 950 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 952 */	NdrFcShort( 0x10 ),	/* 16 */
/* 954 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 956 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 958 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 960 */	
			0x12, 0x0,	/* FC_UP */
/* 962 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (950) */
/* 964 */	
			0x12, 0x8,	/* FC_UP [simple_pointer] */
/* 966 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 968 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 970 */	NdrFcShort( 0x20 ),	/* 32 */
/* 972 */	NdrFcShort( 0x0 ),	/* 0 */
/* 974 */	NdrFcShort( 0x0 ),	/* Offset= 0 (974) */
/* 976 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 978 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 980 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 982 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 984 */	NdrFcShort( 0xfc56 ),	/* Offset= -938 (46) */
/* 986 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 988 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 990 */	NdrFcShort( 0x1 ),	/* 1 */
/* 992 */	NdrFcShort( 0x18 ),	/* 24 */
/* 994 */	NdrFcShort( 0x0 ),	/* 0 */
/* 996 */	NdrFcShort( 0xfc46 ),	/* Offset= -954 (42) */

			0x0
        }
    };

XFG_TRAMPOLINES(BSTR)
XFG_TRAMPOLINES(VARIANT)

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            (USER_MARSHAL_SIZING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserSize)
            ,(USER_MARSHAL_MARSHALLING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserMarshal)
            ,(USER_MARSHAL_UNMARSHALLING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserUnmarshal)
            ,(USER_MARSHAL_FREEING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserFree)
            
            }
            ,
            {
            (USER_MARSHAL_SIZING_ROUTINE)XFG_TRAMPOLINE_FPTR(VARIANT_UserSize)
            ,(USER_MARSHAL_MARSHALLING_ROUTINE)XFG_TRAMPOLINE_FPTR(VARIANT_UserMarshal)
            ,(USER_MARSHAL_UNMARSHALLING_ROUTINE)XFG_TRAMPOLINE_FPTR(VARIANT_UserUnmarshal)
            ,(USER_MARSHAL_FREEING_ROUTINE)XFG_TRAMPOLINE_FPTR(VARIANT_UserFree)
            
            }
            

        };



/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IUdmLinkTransmitter, ver. 0.0,
   GUID={0x14C30E3F,0x910A,0x4B44,{0x90,0x7F,0x2D,0x39,0x24,0x9C,0x1D,0x0C}} */

#pragma code_seg(".orpc")
static const unsigned short IUdmLinkTransmitter_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    86,
    184
    };

static const MIDL_STUBLESS_PROXY_INFO IUdmLinkTransmitter_ProxyInfo =
    {
    &Object_StubDesc,
    Udm2EAutomation__MIDL_ProcFormatString.Format,
    &IUdmLinkTransmitter_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IUdmLinkTransmitter_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    Udm2EAutomation__MIDL_ProcFormatString.Format,
    &IUdmLinkTransmitter_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IUdmLinkTransmitterProxyVtbl = 
{
    &IUdmLinkTransmitter_ProxyInfo,
    &IID_IUdmLinkTransmitter,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IUdmLinkTransmitter::SendLinkToUDM */ ,
    (void *) (INT_PTR) -1 /* IUdmLinkTransmitter::SendLinkToUDM2 */ ,
    (void *) (INT_PTR) -1 /* IUdmLinkTransmitter::SendLinksArray */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IUdmLinkTransmitter_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IUdmLinkTransmitterStubVtbl =
{
    &IID_IUdmLinkTransmitter,
    &IUdmLinkTransmitter_ServerInfo,
    10,
    &IUdmLinkTransmitter_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};

#ifdef __cplusplus
namespace {
#endif
static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    Udm2EAutomation__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x8010274, /* MIDL Version 8.1.628 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };
#ifdef __cplusplus
}
#endif

const CInterfaceProxyVtbl * const _Udm2EAutomation_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IUdmLinkTransmitterProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _Udm2EAutomation_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IUdmLinkTransmitterStubVtbl,
    0
};

PCInterfaceName const _Udm2EAutomation_InterfaceNamesList[] = 
{
    "IUdmLinkTransmitter",
    0
};

const IID *  const _Udm2EAutomation_BaseIIDList[] = 
{
    &IID_IDispatch,
    0
};


#define _Udm2EAutomation_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _Udm2EAutomation, pIID, n)

int __stdcall _Udm2EAutomation_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_Udm2EAutomation_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

EXTERN_C const ExtendedProxyFileInfo Udm2EAutomation_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _Udm2EAutomation_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _Udm2EAutomation_StubVtblList,
    (const PCInterfaceName * ) & _Udm2EAutomation_InterfaceNamesList,
    (const IID ** ) & _Udm2EAutomation_BaseIIDList,
    & _Udm2EAutomation_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

