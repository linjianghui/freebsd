//===-- SBType.cpp ----------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/API/SBDefines.h"
#include "lldb/API/SBType.h"
#include "lldb/API/SBTypeEnumMember.h"
#include "lldb/API/SBStream.h"
#include "lldb/Core/ConstString.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Stream.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Symbol/ClangASTType.h"
#include "lldb/Symbol/Type.h"

#include "clang/AST/Decl.h"

using namespace lldb;
using namespace lldb_private;
using namespace clang;

SBType::SBType() :
    m_opaque_sp()
{
}

SBType::SBType (const ClangASTType &type) :
    m_opaque_sp(new TypeImpl(ClangASTType(type.GetASTContext(),
                                          type.GetOpaqueQualType())))
{
}

SBType::SBType (const lldb::TypeSP &type_sp) :
    m_opaque_sp(new TypeImpl(type_sp))
{
}

SBType::SBType (const lldb::TypeImplSP &type_impl_sp) :
    m_opaque_sp(type_impl_sp)
{
}
    

SBType::SBType (const SBType &rhs) :
    m_opaque_sp()
{
    if (this != &rhs)
    {
        m_opaque_sp = rhs.m_opaque_sp;
    }
}


//SBType::SBType (TypeImpl* impl) :
//    m_opaque_ap(impl)
//{}
//
bool
SBType::operator == (SBType &rhs)
{
    if (IsValid() == false)
        return !rhs.IsValid();
    
    if (rhs.IsValid() == false)
        return false;
    
    return *m_opaque_sp.get() == *rhs.m_opaque_sp.get();
}

bool
SBType::operator != (SBType &rhs)
{    
    if (IsValid() == false)
        return rhs.IsValid();
    
    if (rhs.IsValid() == false)
        return true;
    
    return *m_opaque_sp.get() != *rhs.m_opaque_sp.get();
}

lldb::TypeImplSP
SBType::GetSP ()
{
    return m_opaque_sp;
}


void
SBType::SetSP (const lldb::TypeImplSP &type_impl_sp)
{
    m_opaque_sp = type_impl_sp;
}

SBType &
SBType::operator = (const SBType &rhs)
{
    if (this != &rhs)
    {
        m_opaque_sp = rhs.m_opaque_sp;
    }
    return *this;
}

SBType::~SBType ()
{}

TypeImpl &
SBType::ref ()
{
    if (m_opaque_sp.get() == NULL)
        m_opaque_sp.reset (new TypeImpl());
        return *m_opaque_sp;
}

const TypeImpl &
SBType::ref () const
{
    // "const SBAddress &addr" should already have checked "addr.IsValid()" 
    // prior to calling this function. In case you didn't we will assert
    // and die to let you know.
    assert (m_opaque_sp.get());
    return *m_opaque_sp;
}

bool
SBType::IsValid() const
{
    if (m_opaque_sp.get() == NULL)
        return false;
    
    return m_opaque_sp->IsValid();
}

uint64_t
SBType::GetByteSize()
{
    if (!IsValid())
        return 0;
    
    return m_opaque_sp->GetClangASTType(false).GetByteSize();
    
}

bool
SBType::IsPointerType()
{
    if (!IsValid())
        return false;
    return m_opaque_sp->GetClangASTType(true).IsPointerType();
}

bool
SBType::IsArrayType()
{
    if (!IsValid())
        return false;
    return m_opaque_sp->GetClangASTType(true).IsArrayType(nullptr, nullptr, nullptr);
}

bool
SBType::IsReferenceType()
{
    if (!IsValid())
        return false;
    return m_opaque_sp->GetClangASTType(true).IsReferenceType();
}

SBType
SBType::GetPointerType()
{
    if (!IsValid())
        return SBType();

    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetPointerType())));
}

SBType
SBType::GetPointeeType()
{
    if (!IsValid())
        return SBType();
    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetPointeeType())));
}

SBType
SBType::GetReferenceType()
{
    if (!IsValid())
        return SBType();
    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetReferenceType())));
}

SBType
SBType::GetTypedefedType()
{
    if (!IsValid())
        return SBType();
    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetTypedefedType())));
}

SBType
SBType::GetDereferencedType()
{
    if (!IsValid())
        return SBType();
    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetDereferencedType())));
}

SBType
SBType::GetArrayElementType()
{
    if (!IsValid())
        return SBType();
    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetClangASTType(true).GetArrayElementType())));
}

bool 
SBType::IsFunctionType ()
{
    if (!IsValid())
        return false;
    return m_opaque_sp->GetClangASTType(true).IsFunctionType();
}

bool
SBType::IsPolymorphicClass ()
{
    if (!IsValid())
        return false;
    return m_opaque_sp->GetClangASTType(true).IsPolymorphicClass();
}

bool
SBType::IsTypedefType ()
{
    if (!IsValid())
        return false;
    return m_opaque_sp->GetClangASTType(true).IsTypedefType();
}

lldb::SBType
SBType::GetFunctionReturnType ()
{
    if (IsValid())
    {
        ClangASTType return_clang_type (m_opaque_sp->GetClangASTType(true).GetFunctionReturnType());
        if (return_clang_type.IsValid())
            return SBType(return_clang_type);
    }
    return lldb::SBType();
}

lldb::SBTypeList
SBType::GetFunctionArgumentTypes ()
{
    SBTypeList sb_type_list;
    if (IsValid())
    {
        ClangASTType func_type(m_opaque_sp->GetClangASTType(true));
        size_t count = func_type.GetNumberOfFunctionArguments();
        for (size_t i = 0;
             i < count;
             i++)
        {
            sb_type_list.Append(SBType(func_type.GetFunctionArgumentAtIndex(i)));
        }
    }
    return sb_type_list;
}

uint32_t
SBType::GetNumberOfMemberFunctions ()
{
    if (IsValid())
    {
        return m_opaque_sp->GetClangASTType(true).GetNumMemberFunctions();
    }
    return 0;
}

lldb::SBTypeMemberFunction
SBType::GetMemberFunctionAtIndex (uint32_t idx)
{
    SBTypeMemberFunction sb_func_type;
    if (IsValid())
        sb_func_type.reset(new TypeMemberFunctionImpl(m_opaque_sp->GetClangASTType(true).GetMemberFunctionAtIndex(idx)));
    return sb_func_type;
}

lldb::SBType
SBType::GetUnqualifiedType()
{
    if (!IsValid())
        return SBType();
    return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetUnqualifiedType())));
}

lldb::SBType
SBType::GetCanonicalType()
{
    if (IsValid())
        return SBType(TypeImplSP(new TypeImpl(m_opaque_sp->GetCanonicalType())));
    return SBType();
}


lldb::BasicType
SBType::GetBasicType()
{
    if (IsValid())
        return m_opaque_sp->GetClangASTType(false).GetBasicTypeEnumeration ();
    return eBasicTypeInvalid;
}

SBType
SBType::GetBasicType(lldb::BasicType basic_type)
{
    if (IsValid())
        return SBType (ClangASTContext::GetBasicType (m_opaque_sp->GetClangASTContext(false), basic_type));
    return SBType();
}

uint32_t
SBType::GetNumberOfDirectBaseClasses ()
{
    if (IsValid())
        return m_opaque_sp->GetClangASTType(true).GetNumDirectBaseClasses();
    return 0;
}

uint32_t
SBType::GetNumberOfVirtualBaseClasses ()
{
    if (IsValid())
        return m_opaque_sp->GetClangASTType(true).GetNumVirtualBaseClasses();
    return 0;
}

uint32_t
SBType::GetNumberOfFields ()
{
    if (IsValid())
        return m_opaque_sp->GetClangASTType(true).GetNumFields();
    return 0;
}

bool
SBType::GetDescription (SBStream &description, lldb::DescriptionLevel description_level)
{
    Stream &strm = description.ref();

    if (m_opaque_sp)
    {
        m_opaque_sp->GetDescription (strm, description_level);
    }
    else
        strm.PutCString ("No value");
    
    return true;
}



SBTypeMember
SBType::GetDirectBaseClassAtIndex (uint32_t idx)
{
    SBTypeMember sb_type_member;
    if (IsValid())
    {
        ClangASTType this_type (m_opaque_sp->GetClangASTType (true));
        if (this_type.IsValid())
        {
            uint32_t bit_offset = 0;
            ClangASTType base_class_type (this_type.GetDirectBaseClassAtIndex(idx, &bit_offset));
            if (base_class_type.IsValid())
            {
                sb_type_member.reset (new TypeMemberImpl (TypeImplSP(new TypeImpl(base_class_type)), bit_offset));
            }
        }
    }
    return sb_type_member;

}

SBTypeMember
SBType::GetVirtualBaseClassAtIndex (uint32_t idx)
{
    SBTypeMember sb_type_member;
    if (IsValid())
    {
        ClangASTType this_type (m_opaque_sp->GetClangASTType (true));
        if (this_type.IsValid())
        {
            uint32_t bit_offset = 0;
            ClangASTType base_class_type (this_type.GetVirtualBaseClassAtIndex(idx, &bit_offset));
            if (base_class_type.IsValid())
            {
                sb_type_member.reset (new TypeMemberImpl (TypeImplSP(new TypeImpl(base_class_type)), bit_offset));
            }
        }
    }
    return sb_type_member;
}

SBTypeEnumMemberList
SBType::GetEnumMembers ()
{
    SBTypeEnumMemberList sb_enum_member_list;
    if (IsValid())
    {
        const clang::EnumDecl *enum_decl = m_opaque_sp->GetClangASTType(true).GetFullyUnqualifiedType().GetAsEnumDecl();
        if (enum_decl)
        {
            clang::EnumDecl::enumerator_iterator enum_pos, enum_end_pos;
            for (enum_pos = enum_decl->enumerator_begin(), enum_end_pos = enum_decl->enumerator_end(); enum_pos != enum_end_pos; ++enum_pos)
            {
                SBTypeEnumMember enum_member;
                enum_member.reset(new TypeEnumMemberImpl(*enum_pos, ClangASTType(m_opaque_sp->GetClangASTContext(true), enum_decl->getIntegerType())));
                sb_enum_member_list.Append(enum_member);
            }
        }
    }
    return sb_enum_member_list;
}

SBTypeMember
SBType::GetFieldAtIndex (uint32_t idx)
{
    SBTypeMember sb_type_member;
    if (IsValid())
    {
        ClangASTType this_type (m_opaque_sp->GetClangASTType (false));
        if (this_type.IsValid())
        {
            uint64_t bit_offset = 0;
            uint32_t bitfield_bit_size = 0;
            bool is_bitfield = false;
            std::string name_sstr;
            ClangASTType field_type (this_type.GetFieldAtIndex (idx,
                                                                name_sstr,
                                                                &bit_offset,
                                                                &bitfield_bit_size,
                                                                &is_bitfield));
            if (field_type.IsValid())
            {
                ConstString name;
                if (!name_sstr.empty())
                    name.SetCString(name_sstr.c_str());
                sb_type_member.reset (new TypeMemberImpl (TypeImplSP (new TypeImpl(field_type)),
                                                          bit_offset,
                                                          name,
                                                          bitfield_bit_size,
                                                          is_bitfield));
            }
        }
    }
    return sb_type_member;
}

bool
SBType::IsTypeComplete()
{
    if (!IsValid())
        return false;    
    return m_opaque_sp->GetClangASTType(false).IsCompleteType();
}

uint32_t
SBType::GetTypeFlags ()
{
    if (!IsValid())
        return 0;
    return m_opaque_sp->GetClangASTType(true).GetTypeInfo();
}

const char*
SBType::GetName()
{
    if (!IsValid())
        return "";
    return m_opaque_sp->GetName().GetCString();
}

const char *
SBType::GetDisplayTypeName ()
{
    if (!IsValid())
        return "";
    return m_opaque_sp->GetDisplayTypeName().GetCString();
}

lldb::TypeClass
SBType::GetTypeClass ()
{
    if (IsValid())
        return m_opaque_sp->GetClangASTType(true).GetTypeClass();
    return lldb::eTypeClassInvalid;
}

uint32_t
SBType::GetNumberOfTemplateArguments ()
{
    if (IsValid())
        return m_opaque_sp->GetClangASTType(false).GetNumTemplateArguments();
    return 0;
}

lldb::SBType
SBType::GetTemplateArgumentType (uint32_t idx)
{
    if (IsValid())
    {
        TemplateArgumentKind kind = eTemplateArgumentKindNull;
        ClangASTType template_arg_type = m_opaque_sp->GetClangASTType(false).GetTemplateArgument (idx, kind);
        if (template_arg_type.IsValid())
            return SBType(template_arg_type);
    }
    return SBType();
}


lldb::TemplateArgumentKind
SBType::GetTemplateArgumentKind (uint32_t idx)
{
    TemplateArgumentKind kind = eTemplateArgumentKindNull;
    if (IsValid())
        m_opaque_sp->GetClangASTType(false).GetTemplateArgument (idx, kind);
    return kind;
}


SBTypeList::SBTypeList() :
    m_opaque_ap(new TypeListImpl())
{
}

SBTypeList::SBTypeList(const SBTypeList& rhs) :
    m_opaque_ap(new TypeListImpl())
{
    for (uint32_t i = 0, rhs_size = const_cast<SBTypeList&>(rhs).GetSize(); i < rhs_size; i++)
        Append(const_cast<SBTypeList&>(rhs).GetTypeAtIndex(i));
}

bool
SBTypeList::IsValid ()
{
    return (m_opaque_ap.get() != NULL);
}

SBTypeList&
SBTypeList::operator = (const SBTypeList& rhs)
{
    if (this != &rhs)
    {
        m_opaque_ap.reset (new TypeListImpl());
        for (uint32_t i = 0, rhs_size = const_cast<SBTypeList&>(rhs).GetSize(); i < rhs_size; i++)
            Append(const_cast<SBTypeList&>(rhs).GetTypeAtIndex(i));
    }
    return *this;
}

void
SBTypeList::Append (SBType type)
{
    if (type.IsValid())
        m_opaque_ap->Append (type.m_opaque_sp);
}

SBType
SBTypeList::GetTypeAtIndex(uint32_t index)
{
    if (m_opaque_ap.get())
        return SBType(m_opaque_ap->GetTypeAtIndex(index));
    return SBType();
}

uint32_t
SBTypeList::GetSize()
{
    return m_opaque_ap->GetSize();
}

SBTypeList::~SBTypeList()
{
}

SBTypeMember::SBTypeMember() :
    m_opaque_ap()
{
}

SBTypeMember::~SBTypeMember()
{
}

SBTypeMember::SBTypeMember (const SBTypeMember& rhs) :
    m_opaque_ap()
{
    if (this != &rhs)
    {
        if (rhs.IsValid())
            m_opaque_ap.reset(new TypeMemberImpl(rhs.ref()));
    }
}

lldb::SBTypeMember&
SBTypeMember::operator = (const lldb::SBTypeMember& rhs)
{
    if (this != &rhs)
    {
        if (rhs.IsValid())
            m_opaque_ap.reset(new TypeMemberImpl(rhs.ref()));
    }
    return *this;
}

bool
SBTypeMember::IsValid() const
{
    return m_opaque_ap.get();
}

const char *
SBTypeMember::GetName ()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetName().GetCString();
    return NULL;
}

SBType
SBTypeMember::GetType ()
{
    SBType sb_type;
    if (m_opaque_ap.get())
    {
        sb_type.SetSP (m_opaque_ap->GetTypeImpl());
    }
    return sb_type;

}

uint64_t
SBTypeMember::GetOffsetInBytes()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetBitOffset() / 8u;
    return 0;
}

uint64_t
SBTypeMember::GetOffsetInBits()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetBitOffset();
    return 0;
}

bool
SBTypeMember::IsBitfield()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetIsBitfield();
    return false;
}

uint32_t
SBTypeMember::GetBitfieldSizeInBits()
{
    if (m_opaque_ap.get())
        return m_opaque_ap->GetBitfieldBitSize();
    return 0;
}


bool
SBTypeMember::GetDescription (lldb::SBStream &description, lldb::DescriptionLevel description_level)
{
    Stream &strm = description.ref();

    if (m_opaque_ap.get())
    {
        const uint32_t bit_offset = m_opaque_ap->GetBitOffset();
        const uint32_t byte_offset = bit_offset / 8u;
        const uint32_t byte_bit_offset = bit_offset % 8u;
        const char *name = m_opaque_ap->GetName().GetCString();
        if (byte_bit_offset)
            strm.Printf ("+%u + %u bits: (", byte_offset, byte_bit_offset);
        else
            strm.Printf ("+%u: (", byte_offset);
        
        TypeImplSP type_impl_sp (m_opaque_ap->GetTypeImpl());
        if (type_impl_sp)
            type_impl_sp->GetDescription(strm, description_level);
        
        strm.Printf (") %s", name);
        if (m_opaque_ap->GetIsBitfield())
        {
            const uint32_t bitfield_bit_size = m_opaque_ap->GetBitfieldBitSize();
            strm.Printf (" : %u", bitfield_bit_size);
        }
    }
    else
    {
        strm.PutCString ("No value");
    }
    return true;   
}


void
SBTypeMember::reset(TypeMemberImpl *type_member_impl)
{
    m_opaque_ap.reset(type_member_impl);
}

TypeMemberImpl &
SBTypeMember::ref ()
{
    if (m_opaque_ap.get() == NULL)
        m_opaque_ap.reset (new TypeMemberImpl());
    return *m_opaque_ap.get();
}

const TypeMemberImpl &
SBTypeMember::ref () const
{
    return *m_opaque_ap.get();
}

SBTypeMemberFunction::SBTypeMemberFunction() :
m_opaque_sp()
{
}

SBTypeMemberFunction::~SBTypeMemberFunction()
{
}

SBTypeMemberFunction::SBTypeMemberFunction (const SBTypeMemberFunction& rhs) :
    m_opaque_sp(rhs.m_opaque_sp)
{
}

lldb::SBTypeMemberFunction&
SBTypeMemberFunction::operator = (const lldb::SBTypeMemberFunction& rhs)
{
    if (this != &rhs)
        m_opaque_sp = rhs.m_opaque_sp;
    return *this;
}

bool
SBTypeMemberFunction::IsValid() const
{
    return m_opaque_sp.get();
}

const char *
SBTypeMemberFunction::GetName ()
{
    if (m_opaque_sp)
        return m_opaque_sp->GetName().GetCString();
    return NULL;
}

SBType
SBTypeMemberFunction::GetType ()
{
    SBType sb_type;
    if (m_opaque_sp)
    {
        sb_type.SetSP(lldb::TypeImplSP(new TypeImpl(m_opaque_sp->GetType())));
    }
    return sb_type;
}

lldb::SBType
SBTypeMemberFunction::GetReturnType ()
{
    SBType sb_type;
    if (m_opaque_sp)
    {
        sb_type.SetSP(lldb::TypeImplSP(new TypeImpl(m_opaque_sp->GetReturnType())));
    }
    return sb_type;
}

uint32_t
SBTypeMemberFunction::GetNumberOfArguments ()
{
    if (m_opaque_sp)
        return m_opaque_sp->GetNumArguments();
    return 0;
}

lldb::SBType
SBTypeMemberFunction::GetArgumentTypeAtIndex (uint32_t i)
{
    SBType sb_type;
    if (m_opaque_sp)
    {
        sb_type.SetSP(lldb::TypeImplSP(new TypeImpl(m_opaque_sp->GetArgumentAtIndex(i))));
    }
    return sb_type;
}

lldb::MemberFunctionKind
SBTypeMemberFunction::GetKind ()
{
    if (m_opaque_sp)
        return m_opaque_sp->GetKind();
    return lldb::eMemberFunctionKindUnknown;
    
}

bool
SBTypeMemberFunction::GetDescription (lldb::SBStream &description,
                                      lldb::DescriptionLevel description_level)
{
    Stream &strm = description.ref();
    
    if (m_opaque_sp)
        return m_opaque_sp->GetDescription(strm);
    
    return false;
}

void
SBTypeMemberFunction::reset(TypeMemberFunctionImpl *type_member_impl)
{
    m_opaque_sp.reset(type_member_impl);
}

TypeMemberFunctionImpl &
SBTypeMemberFunction::ref ()
{
    if (!m_opaque_sp)
        m_opaque_sp.reset (new TypeMemberFunctionImpl());
    return *m_opaque_sp.get();
}

const TypeMemberFunctionImpl &
SBTypeMemberFunction::ref () const
{
    return *m_opaque_sp.get();
}
