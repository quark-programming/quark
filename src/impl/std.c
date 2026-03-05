#include "std.h"

Scope global_std_scope = { 0 };

static Declaration* build_type(const str name, const str display, const u32 flags) {
    Type* type = new_type((Type) {
        .External = {
            .id = NodeExternal,
            .trace.source = display,
            .flags = fConst | fConstExpr | fType | flags,
            .data = name,
        },
    });

    return (void*) new_node((Node) {
        .Declaration = {
            .id = NodeNone,
            .flags = fConst | fConstExpr | fType,
            .type = type,
            .const_value = (void*) type,
        },
    });
}

static void put_std_type(const str identifier, const str name, const u32 flags) {
    put(&global_std_scope.variables, identifier, build_type(name, identifier, flags));
}

static Declaration* build_const(const str name, const u32 flags, Type* const type) {
    return (void*) new_node((Node) {
        .Declaration = {
            .id = NodeNone,
            .flags = fConst | fConstExpr | fType,
            .type = type,
            .const_value = new_node((Node) {
                .External = {
                    .id = NodeExternal,
                    .flags = fConst | fConstExpr | fType | flags,
                    .type = type,
                    .data = name,
                },
            }),
        },
    });
}

static void put_std_const(const str identifier, const str name, const u32 flags, Type* const type) {
    put(&global_std_scope.variables, identifier, build_const(name, flags, type));
}

void build_std_scope(Declaration* declaration) {
    global_std_scope.declaration = declaration;

    put_std_type(str("i8"), str("int8_t"), fNumeric);
    put_std_type(str("u8"), str("uint8_t"), fNumeric);
    put_std_type(str("i16"), str("int16_t"), fNumeric);
    put_std_type(str("u16"), str("uint16_t"), fNumeric);
    put_std_type(str("i32"), str("int32_t"), fNumeric);
    put_std_type(str("u32"), str("uint32_t"), fNumeric);
    put_std_type(str("i64"), str("int64_t"), fNumeric);
    put_std_type(str("u64"), str("uint64_t"), fNumeric);

    put_std_type(str("f32"), str("float"), fNumeric);
    put_std_type(str("f64"), str("double"), fNumeric);

    put_std_type(str("isize"), str("ptrdiff_t"), fNumeric);
    put_std_type(str("usize"), str("size_t"), fNumeric);

    put_std_type(str("char"), str("char"), fNumeric);
    put_std_type(str("ichar"), str("signed char"), fNumeric);
    put_std_type(str("uchar"), str("unsigned char"), fNumeric);

    put_std_type(str("Short"), str("short"), fNumeric);
    put_std_type(str("UShort"), str("unsigned short"), fNumeric);
    put_std_type(str("Int"), str("int"), fNumeric);
    put_std_type(str("UInt"), str("unsigned int"), fNumeric);
    put_std_type(str("Long"), str("long"), fNumeric);
    put_std_type(str("ULong"), str("unsigned long"), fNumeric);
    put_std_type(str("LongLong"), str("long long"), fNumeric);
    put_std_type(str("ULongLong"), str("unsigned long long"), fNumeric);

    put_std_type(str("bool"), str("bool"), fNumeric);
    // put_std_type(str("File"), str("FILE"), 0);
    put_std_type(str("void"), str("void"), 0);

    static Type bool_type = {
        .External = {
            .id = NodeExternal,
            .flags = fConst | fConstExpr | fType | fNumeric,
            .type = &bool_type,
            .data = str("bool"),
        },
    };
    static Type const_auto = {
        .Wrapper = {
            .id = WrapperAuto,
            .flags = fConst | fConstExpr | fType,
            .type = &const_auto,
            .Auto.constant = true,
        },
    };
    static Type const_autop = {
        .PointerType = {
            .id = NodePointerType,
            .flags = fConst | fConstExpr | fType | fNumeric,
            .type = &const_autop,
            .base = &const_auto,
        },
    };

    put_std_const(str("true"), str("true"), fNumeric, &bool_type);
    put_std_const(str("false"), str("false"), fNumeric, &bool_type);
    put_std_const(str("null"), str("NULL"), fNumeric, &const_autop);
}
