/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "builtin/ModuleObject.h"

#include "mozilla/EnumSet.h"

#include "builtin/SelfHostingDefines.h"
#include "frontend/ParseNode.h"
#include "frontend/SharedContext.h"
#include "gc/Policy.h"
#include "gc/Tracer.h"
#include "vm/AsyncFunction.h"
#include "vm/AsyncIteration.h"

#include "jsobjinlines.h"
#include "jsscriptinlines.h"

using namespace js;
using namespace js::frontend;

static_assert(MODULE_STATUS_ERRORED < MODULE_STATUS_UNINSTANTIATED &&
              MODULE_STATUS_UNINSTANTIATED < MODULE_STATUS_INSTANTIATING &&
              MODULE_STATUS_INSTANTIATING < MODULE_STATUS_INSTANTIATED &&
              MODULE_STATUS_INSTANTIATED < MODULE_STATUS_EVALUATED,
              "Module statuses are ordered incorrectly");

template<typename T, Value ValueGetter(const T* obj)>
static bool
ModuleValueGetterImpl(JSContext* cx, const CallArgs& args)
{
    args.rval().set(ValueGetter(&args.thisv().toObject().as<T>()));
    return true;
}

template<typename T, Value ValueGetter(const T* obj)>
static bool
ModuleValueGetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<T::isInstance, ModuleValueGetterImpl<T, ValueGetter>>(cx, args);
}

#define DEFINE_GETTER_FUNCTIONS(cls, name, slot)                              \
    static Value                                                              \
    cls##_##name##Value(const cls* obj) {                                     \
        return obj->getReservedSlot(cls::slot);                               \
    }                                                                         \
                                                                              \
    static bool                                                               \
    cls##_##name##Getter(JSContext* cx, unsigned argc, Value* vp)             \
    {                                                                         \
        return ModuleValueGetter<cls, cls##_##name##Value>(cx, argc, vp);     \
    }

#define DEFINE_ATOM_ACCESSOR_METHOD(cls, name)                                \
    JSAtom*                                                                   \
    cls::name() const                                                         \
    {                                                                         \
        Value value = cls##_##name##Value(this);                              \
        return &value.toString()->asAtom();                                   \
    }

#define DEFINE_ATOM_OR_NULL_ACCESSOR_METHOD(cls, name)                        \
    JSAtom*                                                                   \
    cls::name() const                                                         \
    {                                                                         \
        Value value = cls##_##name##Value(this);                              \
        if (value.isNull())                                                   \
            return nullptr;                                                   \
        return &value.toString()->asAtom();                                   \
    }

#define DEFINE_UINT32_ACCESSOR_METHOD(cls, name)                              \
    uint32_t                                                                  \
    cls::name() const                                                         \
    {                                                                         \
        Value value = cls##_##name##Value(this);                              \
        MOZ_ASSERT(value.toInt32() >= 0);                                     \
        return value.toInt32();                                               \
    }

///////////////////////////////////////////////////////////////////////////
// ImportEntryObject

/* static */ const Class
ImportEntryObject::class_ = {
    "ImportEntry",
    JSCLASS_HAS_RESERVED_SLOTS(ImportEntryObject::SlotCount) |
    JSCLASS_IS_ANONYMOUS
};

DEFINE_GETTER_FUNCTIONS(ImportEntryObject, moduleRequest, ModuleRequestSlot)
DEFINE_GETTER_FUNCTIONS(ImportEntryObject, importName, ImportNameSlot)
DEFINE_GETTER_FUNCTIONS(ImportEntryObject, localName, LocalNameSlot)
DEFINE_GETTER_FUNCTIONS(ImportEntryObject, lineNumber, LineNumberSlot)
DEFINE_GETTER_FUNCTIONS(ImportEntryObject, columnNumber, ColumnNumberSlot)

DEFINE_ATOM_ACCESSOR_METHOD(ImportEntryObject, moduleRequest)
DEFINE_ATOM_ACCESSOR_METHOD(ImportEntryObject, importName)
DEFINE_ATOM_ACCESSOR_METHOD(ImportEntryObject, localName)
DEFINE_UINT32_ACCESSOR_METHOD(ImportEntryObject, lineNumber)
DEFINE_UINT32_ACCESSOR_METHOD(ImportEntryObject, columnNumber)

/* static */ bool
ImportEntryObject::isInstance(HandleValue value)
{
    return value.isObject() && value.toObject().is<ImportEntryObject>();
}

/* static */ bool
GlobalObject::initImportEntryProto(JSContext* cx, Handle<GlobalObject*> global)
{
    static const JSPropertySpec protoAccessors[] = {
        JS_PSG("moduleRequest", ImportEntryObject_moduleRequestGetter, 0),
        JS_PSG("importName", ImportEntryObject_importNameGetter, 0),
        JS_PSG("localName", ImportEntryObject_localNameGetter, 0),
        JS_PSG("lineNumber", ImportEntryObject_lineNumberGetter, 0),
        JS_PSG("columnNumber", ImportEntryObject_columnNumberGetter, 0),
        JS_PS_END
    };

    RootedObject proto(cx, GlobalObject::createBlankPrototype<PlainObject>(cx, global));
    if (!proto)
        return false;

    if (!DefinePropertiesAndFunctions(cx, proto, protoAccessors, nullptr))
        return false;

    global->initReservedSlot(IMPORT_ENTRY_PROTO, ObjectValue(*proto));
    return true;
}

/* static */ ImportEntryObject*
ImportEntryObject::create(JSContext* cx,
                          HandleAtom moduleRequest,
                          HandleAtom importName,
                          HandleAtom localName,
                          uint32_t lineNumber,
                          uint32_t columnNumber)
{
    MOZ_ASSERT(lineNumber > 0);

    RootedObject proto(cx, cx->global()->getImportEntryPrototype());
    RootedObject obj(cx, NewObjectWithGivenProto(cx, &class_, proto));
    if (!obj)
        return nullptr;

    RootedImportEntryObject self(cx, &obj->as<ImportEntryObject>());
    self->initReservedSlot(ModuleRequestSlot, StringValue(moduleRequest));
    self->initReservedSlot(ImportNameSlot, StringValue(importName));
    self->initReservedSlot(LocalNameSlot, StringValue(localName));
    self->initReservedSlot(LineNumberSlot, Int32Value(lineNumber));
    self->initReservedSlot(ColumnNumberSlot, Int32Value(columnNumber));
    return self;
}

///////////////////////////////////////////////////////////////////////////
// ExportEntryObject

/* static */ const Class
ExportEntryObject::class_ = {
    "ExportEntry",
    JSCLASS_HAS_RESERVED_SLOTS(ExportEntryObject::SlotCount) |
    JSCLASS_IS_ANONYMOUS
};

DEFINE_GETTER_FUNCTIONS(ExportEntryObject, exportName, ExportNameSlot)
DEFINE_GETTER_FUNCTIONS(ExportEntryObject, moduleRequest, ModuleRequestSlot)
DEFINE_GETTER_FUNCTIONS(ExportEntryObject, importName, ImportNameSlot)
DEFINE_GETTER_FUNCTIONS(ExportEntryObject, localName, LocalNameSlot)
DEFINE_GETTER_FUNCTIONS(ExportEntryObject, lineNumber, LineNumberSlot)
DEFINE_GETTER_FUNCTIONS(ExportEntryObject, columnNumber, ColumnNumberSlot)

DEFINE_ATOM_OR_NULL_ACCESSOR_METHOD(ExportEntryObject, exportName)
DEFINE_ATOM_OR_NULL_ACCESSOR_METHOD(ExportEntryObject, moduleRequest)
DEFINE_ATOM_OR_NULL_ACCESSOR_METHOD(ExportEntryObject, importName)
DEFINE_ATOM_OR_NULL_ACCESSOR_METHOD(ExportEntryObject, localName)
DEFINE_UINT32_ACCESSOR_METHOD(ExportEntryObject, lineNumber)
DEFINE_UINT32_ACCESSOR_METHOD(ExportEntryObject, columnNumber)

/* static */ bool
ExportEntryObject::isInstance(HandleValue value)
{
    return value.isObject() && value.toObject().is<ExportEntryObject>();
}

/* static */ bool
GlobalObject::initExportEntryProto(JSContext* cx, Handle<GlobalObject*> global)
{
    static const JSPropertySpec protoAccessors[] = {
        JS_PSG("exportName", ExportEntryObject_exportNameGetter, 0),
        JS_PSG("moduleRequest", ExportEntryObject_moduleRequestGetter, 0),
        JS_PSG("importName", ExportEntryObject_importNameGetter, 0),
        JS_PSG("localName", ExportEntryObject_localNameGetter, 0),
        JS_PSG("lineNumber", ExportEntryObject_lineNumberGetter, 0),
        JS_PSG("columnNumber", ExportEntryObject_columnNumberGetter, 0),
        JS_PS_END
    };

    RootedObject proto(cx, GlobalObject::createBlankPrototype<PlainObject>(cx, global));
    if (!proto)
        return false;

    if (!DefinePropertiesAndFunctions(cx, proto, protoAccessors, nullptr))
        return false;

    global->initReservedSlot(EXPORT_ENTRY_PROTO, ObjectValue(*proto));
    return true;
}

static Value
StringOrNullValue(JSString* maybeString)
{
    return maybeString ? StringValue(maybeString) : NullValue();
}

/* static */ ExportEntryObject*
ExportEntryObject::create(JSContext* cx,
                          HandleAtom maybeExportName,
                          HandleAtom maybeModuleRequest,
                          HandleAtom maybeImportName,
                          HandleAtom maybeLocalName,
                          uint32_t lineNumber,
                          uint32_t columnNumber)
{
    // Line and column numbers are optional for export entries since direct
    // entries are checked at parse time.

    RootedObject proto(cx, cx->global()->getExportEntryPrototype());
    RootedObject obj(cx, NewObjectWithGivenProto(cx, &class_, proto));
    if (!obj)
        return nullptr;

    RootedExportEntryObject self(cx, &obj->as<ExportEntryObject>());
    self->initReservedSlot(ExportNameSlot, StringOrNullValue(maybeExportName));
    self->initReservedSlot(ModuleRequestSlot, StringOrNullValue(maybeModuleRequest));
    self->initReservedSlot(ImportNameSlot, StringOrNullValue(maybeImportName));
    self->initReservedSlot(LocalNameSlot, StringOrNullValue(maybeLocalName));
    self->initReservedSlot(LineNumberSlot, Int32Value(lineNumber));
    self->initReservedSlot(ColumnNumberSlot, Int32Value(columnNumber));
    return self;
}

///////////////////////////////////////////////////////////////////////////
// RequestedModuleObject

/* static */ const Class
RequestedModuleObject::class_ = {
    "RequestedModule",
    JSCLASS_HAS_RESERVED_SLOTS(RequestedModuleObject::SlotCount) |
    JSCLASS_IS_ANONYMOUS
};

DEFINE_GETTER_FUNCTIONS(RequestedModuleObject, moduleSpecifier, ModuleSpecifierSlot)
DEFINE_GETTER_FUNCTIONS(RequestedModuleObject, lineNumber, LineNumberSlot)
DEFINE_GETTER_FUNCTIONS(RequestedModuleObject, columnNumber, ColumnNumberSlot)

DEFINE_ATOM_ACCESSOR_METHOD(RequestedModuleObject, moduleSpecifier)
DEFINE_UINT32_ACCESSOR_METHOD(RequestedModuleObject, lineNumber)
DEFINE_UINT32_ACCESSOR_METHOD(RequestedModuleObject, columnNumber)

/* static */ bool
RequestedModuleObject::isInstance(HandleValue value)
{
    return value.isObject() && value.toObject().is<RequestedModuleObject>();
}

/* static */ bool
GlobalObject::initRequestedModuleProto(JSContext* cx, Handle<GlobalObject*> global)
{
    static const JSPropertySpec protoAccessors[] = {
        JS_PSG("moduleSpecifier", RequestedModuleObject_moduleSpecifierGetter, 0),
        JS_PSG("lineNumber", RequestedModuleObject_lineNumberGetter, 0),
        JS_PSG("columnNumber", RequestedModuleObject_columnNumberGetter, 0),
        JS_PS_END
    };

    RootedObject proto(cx, GlobalObject::createBlankPrototype<PlainObject>(cx, global));
    if (!proto)
        return false;

    if (!DefinePropertiesAndFunctions(cx, proto, protoAccessors, nullptr))
        return false;

    global->initReservedSlot(REQUESTED_MODULE_PROTO, ObjectValue(*proto));
    return true;
}

/* static */ RequestedModuleObject*
RequestedModuleObject::create(JSContext* cx,
                              HandleAtom moduleSpecifier,
                              uint32_t lineNumber,
                              uint32_t columnNumber)
{
    MOZ_ASSERT(lineNumber > 0);

    RootedObject proto(cx, cx->global()->getRequestedModulePrototype());
    RootedObject obj(cx, NewObjectWithGivenProto(cx, &class_, proto));
    if (!obj)
        return nullptr;

    RootedRequestedModuleObject self(cx, &obj->as<RequestedModuleObject>());
    self->initReservedSlot(ModuleSpecifierSlot, StringValue(moduleSpecifier));
    self->initReservedSlot(LineNumberSlot, Int32Value(lineNumber));
    self->initReservedSlot(ColumnNumberSlot, Int32Value(columnNumber));
    return self;
}

///////////////////////////////////////////////////////////////////////////
// IndirectBindingMap

IndirectBindingMap::Binding::Binding(ModuleEnvironmentObject* environment, Shape* shape)
  : environment(environment), shape(shape)
{}

IndirectBindingMap::IndirectBindingMap(Zone* zone)
  : map_(ZoneAllocPolicy(zone))
{
}

bool
IndirectBindingMap::init()
{
    return map_.init();
}

void
IndirectBindingMap::trace(JSTracer* trc)
{
    for (Map::Enum e(map_); !e.empty(); e.popFront()) {
        Binding& b = e.front().value();
        TraceEdge(trc, &b.environment, "module bindings environment");
        TraceEdge(trc, &b.shape, "module bindings shape");
        jsid bindingName = e.front().key();
        TraceManuallyBarrieredEdge(trc, &bindingName, "module bindings binding name");
        MOZ_ASSERT(bindingName == e.front().key());
    }
}

bool
IndirectBindingMap::putNew(JSContext* cx, HandleId name,
                           HandleModuleEnvironmentObject environment, HandleId localName)
{
    RootedShape shape(cx, environment->lookup(cx, localName));
    MOZ_ASSERT(shape);
    if (!map_.putNew(name, Binding(environment, shape))) {
        ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

bool
IndirectBindingMap::lookup(jsid name, ModuleEnvironmentObject** envOut, Shape** shapeOut) const
{
    auto ptr = map_.lookup(name);
    if (!ptr)
        return false;

    const Binding& binding = ptr->value();
    MOZ_ASSERT(binding.environment);
    MOZ_ASSERT(!binding.environment->inDictionaryMode());
    MOZ_ASSERT(binding.environment->containsPure(binding.shape));
    *envOut = binding.environment;
    *shapeOut = binding.shape;
    return true;
}

///////////////////////////////////////////////////////////////////////////
// ModuleNamespaceObject

/* static */ const ModuleNamespaceObject::ProxyHandler ModuleNamespaceObject::proxyHandler;

/* static */ bool
ModuleNamespaceObject::isInstance(HandleValue value)
{
    return value.isObject() && value.toObject().is<ModuleNamespaceObject>();
}

/* static */ ModuleNamespaceObject*
ModuleNamespaceObject::create(JSContext* cx, HandleModuleObject module)
{
    RootedValue priv(cx, ObjectValue(*module));
    ProxyOptions options;
    options.setLazyProto(true);
    options.setSingleton(true);
    RootedObject object(cx, NewProxyObject(cx, &proxyHandler, priv, nullptr, options));
    if (!object)
        return nullptr;

    return &object->as<ModuleNamespaceObject>();
}

ModuleObject&
ModuleNamespaceObject::module()
{
    return GetProxyPrivate(this).toObject().as<ModuleObject>();
}

JSObject&
ModuleNamespaceObject::exports()
{
    JSObject* exports = module().namespaceExports();
    MOZ_ASSERT(exports);
    return *exports;
}

IndirectBindingMap&
ModuleNamespaceObject::bindings()
{
    IndirectBindingMap* bindings = module().namespaceBindings();
    MOZ_ASSERT(bindings);
    return *bindings;
}

bool
ModuleNamespaceObject::addBinding(JSContext* cx, HandleAtom exportedName,
                                  HandleModuleObject targetModule, HandleAtom localName)
{
    IndirectBindingMap* bindings(this->module().namespaceBindings());
    MOZ_ASSERT(bindings);

    RootedModuleEnvironmentObject environment(cx, &targetModule->initialEnvironment());
    RootedId exportedNameId(cx, AtomToId(exportedName));
    RootedId localNameId(cx, AtomToId(localName));
    return bindings->putNew(cx, exportedNameId, environment, localNameId);
}

const char ModuleNamespaceObject::ProxyHandler::family = 0;

ModuleNamespaceObject::ProxyHandler::ProxyHandler()
  : BaseProxyHandler(&family, false)
{}

bool
ModuleNamespaceObject::ProxyHandler::getPrototype(JSContext* cx, HandleObject proxy,
                                                  MutableHandleObject protop) const
{
    protop.set(nullptr);
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::setPrototype(JSContext* cx, HandleObject proxy,
                                                  HandleObject proto, ObjectOpResult& result) const
{
    if (!proto)
        return result.succeed();
    return result.failCantSetProto();
}

bool
ModuleNamespaceObject::ProxyHandler::getPrototypeIfOrdinary(JSContext* cx, HandleObject proxy,
                                                            bool* isOrdinary,
                                                            MutableHandleObject protop) const
{
    *isOrdinary = false;
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::setImmutablePrototype(JSContext* cx, HandleObject proxy,
                                                           bool* succeeded) const
{
    *succeeded = true;
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::isExtensible(JSContext* cx, HandleObject proxy,
                                                  bool* extensible) const
{
    *extensible = false;
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::preventExtensions(JSContext* cx, HandleObject proxy,
                                                 ObjectOpResult& result) const
{
    result.succeed();
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::getOwnPropertyDescriptor(JSContext* cx, HandleObject proxy,
                                                              HandleId id,
                                                              MutableHandle<PropertyDescriptor> desc) const
{
    Rooted<ModuleNamespaceObject*> ns(cx, &proxy->as<ModuleNamespaceObject>());
    if (JSID_IS_SYMBOL(id)) {
        if (JSID_TO_SYMBOL(id) == cx->wellKnownSymbols().toStringTag) {
            RootedValue value(cx, StringValue(cx->names().Module));
            desc.object().set(proxy);
            desc.setWritable(false);
            desc.setEnumerable(false);
            desc.setConfigurable(false);
            desc.setValue(value);
            return true;
        }

        return true;
    }

    const IndirectBindingMap& bindings = ns->bindings();
    ModuleEnvironmentObject* env;
    Shape* shape;
    if (!bindings.lookup(id, &env, &shape))
        return true;

    RootedValue value(cx, env->getSlot(shape->slot()));
    if (value.isMagic(JS_UNINITIALIZED_LEXICAL)) {
        ReportRuntimeLexicalError(cx, JSMSG_UNINITIALIZED_LEXICAL, id);
        return false;
    }

    desc.object().set(env);
    desc.setConfigurable(false);
    desc.setEnumerable(true);
    desc.setValue(value);
    return true;
}

static bool
ValidatePropertyDescriptor(JSContext* cx, Handle<PropertyDescriptor> desc,
                           bool expectedWritable, bool expectedEnumerable,
                           bool expectedConfigurable, HandleValue expectedValue,
                           ObjectOpResult& result)
{
    if (desc.isAccessorDescriptor())
        return result.fail(JSMSG_CANT_REDEFINE_PROP);

    if (desc.hasWritable() && desc.writable() != expectedWritable)
        return result.fail(JSMSG_CANT_REDEFINE_PROP);

    if (desc.hasEnumerable() && desc.enumerable() != expectedEnumerable)
        return result.fail(JSMSG_CANT_REDEFINE_PROP);

    if (desc.hasConfigurable() && desc.configurable() != expectedConfigurable)
        return result.fail(JSMSG_CANT_REDEFINE_PROP);

    if (desc.hasValue()) {
        bool same;
        if (!SameValue(cx, desc.value(), expectedValue, &same))
            return false;
        if (!same)
            return result.fail(JSMSG_CANT_REDEFINE_PROP);
    }

    return result.succeed();
}

bool
ModuleNamespaceObject::ProxyHandler::defineProperty(JSContext* cx, HandleObject proxy, HandleId id,
                                                    Handle<PropertyDescriptor> desc,
                                                    ObjectOpResult& result) const
{
    if (JSID_IS_SYMBOL(id)) {
        if (JSID_TO_SYMBOL(id) == cx->wellKnownSymbols().toStringTag) {
            RootedValue value(cx, StringValue(cx->names().Module));
            return ValidatePropertyDescriptor(cx, desc, false, false, false, value, result);
        }
        return result.fail(JSMSG_CANT_DEFINE_PROP_OBJECT_NOT_EXTENSIBLE);
    }

    const IndirectBindingMap& bindings = proxy->as<ModuleNamespaceObject>().bindings();
    ModuleEnvironmentObject* env;
    Shape* shape;
    if (!bindings.lookup(id, &env, &shape))
        return result.fail(JSMSG_CANT_DEFINE_PROP_OBJECT_NOT_EXTENSIBLE);

    RootedValue value(cx, env->getSlot(shape->slot()));
    if (value.isMagic(JS_UNINITIALIZED_LEXICAL)) {
        ReportRuntimeLexicalError(cx, JSMSG_UNINITIALIZED_LEXICAL, id);
        return false;
    }

    return ValidatePropertyDescriptor(cx, desc, true, true, false, value, result);
}

bool
ModuleNamespaceObject::ProxyHandler::has(JSContext* cx, HandleObject proxy, HandleId id,
                                         bool* bp) const
{
    Rooted<ModuleNamespaceObject*> ns(cx, &proxy->as<ModuleNamespaceObject>());
    if (JSID_IS_SYMBOL(id)) {
        *bp = JSID_TO_SYMBOL(id) == cx->wellKnownSymbols().toStringTag;
        return true;
    }

    *bp = ns->bindings().has(id);
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::get(JSContext* cx, HandleObject proxy, HandleValue receiver,
                                         HandleId id, MutableHandleValue vp) const
{
    Rooted<ModuleNamespaceObject*> ns(cx, &proxy->as<ModuleNamespaceObject>());
    if (JSID_IS_SYMBOL(id)) {
        if (JSID_TO_SYMBOL(id) == cx->wellKnownSymbols().toStringTag) {
            vp.setString(cx->names().Module);
            return true;
        }

        vp.setUndefined();
        return true;
    }

    ModuleEnvironmentObject* env;
    Shape* shape;
    if (!ns->bindings().lookup(id, &env, &shape)) {
        vp.setUndefined();
        return true;
    }

    RootedValue value(cx, env->getSlot(shape->slot()));
    if (value.isMagic(JS_UNINITIALIZED_LEXICAL)) {
        ReportRuntimeLexicalError(cx, JSMSG_UNINITIALIZED_LEXICAL, id);
        return false;
    }

    vp.set(value);
    return true;
}

bool
ModuleNamespaceObject::ProxyHandler::set(JSContext* cx, HandleObject proxy, HandleId id, HandleValue v,
                                         HandleValue receiver, ObjectOpResult& result) const
{
    return result.failReadOnly();
}

bool
ModuleNamespaceObject::ProxyHandler::delete_(JSContext* cx, HandleObject proxy, HandleId id,
                                             ObjectOpResult& result) const
{
    Rooted<ModuleNamespaceObject*> ns(cx, &proxy->as<ModuleNamespaceObject>());
    if (JSID_IS_SYMBOL(id)) {
        if (JSID_TO_SYMBOL(id) == cx->wellKnownSymbols().toStringTag)
            return result.failCantDelete();

        return result.succeed();
    }

    if (ns->bindings().has(id))
        return result.failCantDelete();

    return result.succeed();
}

bool
ModuleNamespaceObject::ProxyHandler::ownPropertyKeys(JSContext* cx, HandleObject proxy,
                                                     AutoIdVector& props) const
{
    Rooted<ModuleNamespaceObject*> ns(cx, &proxy->as<ModuleNamespaceObject>());
    RootedObject exports(cx, &ns->exports());
    uint32_t count;
    if (!GetLengthProperty(cx, exports, &count) || !props.reserve(props.length() + count + 1))
        return false;

    Rooted<ValueVector> names(cx, ValueVector(cx));
    if (!names.resize(count) || !GetElements(cx, exports, count, names.begin()))
        return false;

    for (uint32_t i = 0; i < count; i++)
        props.infallibleAppend(AtomToId(&names[i].toString()->asAtom()));

    props.infallibleAppend(SYMBOL_TO_JSID(cx->wellKnownSymbols().toStringTag));

    return true;
}

///////////////////////////////////////////////////////////////////////////
// FunctionDeclaration

FunctionDeclaration::FunctionDeclaration(HandleAtom name, HandleFunction fun)
  : name(name), fun(fun)
{}

void FunctionDeclaration::trace(JSTracer* trc)
{
    TraceEdge(trc, &name, "FunctionDeclaration name");
    TraceEdge(trc, &fun, "FunctionDeclaration fun");
}

///////////////////////////////////////////////////////////////////////////
// ModuleObject

/* static */ const ClassOps
ModuleObject::classOps_ = {
    nullptr,        /* addProperty */
    nullptr,        /* delProperty */
    nullptr,        /* enumerate   */
    nullptr,        /* newEnumerate */
    nullptr,        /* resolve     */
    nullptr,        /* mayResolve  */
    ModuleObject::finalize,
    nullptr,        /* call        */
    nullptr,        /* hasInstance */
    nullptr,        /* construct   */
    ModuleObject::trace
};

/* static */ const Class
ModuleObject::class_ = {
    "Module",
    JSCLASS_HAS_RESERVED_SLOTS(ModuleObject::SlotCount) |
    JSCLASS_IS_ANONYMOUS |
    JSCLASS_BACKGROUND_FINALIZE,
    &ModuleObject::classOps_
};

#define DEFINE_ARRAY_SLOT_ACCESSOR(cls, name, slot)                           \
    ArrayObject&                                                              \
    cls::name() const                                                         \
    {                                                                         \
        return getReservedSlot(cls::slot).toObject().as<ArrayObject>();       \
    }

DEFINE_ARRAY_SLOT_ACCESSOR(ModuleObject, requestedModules, RequestedModulesSlot)
DEFINE_ARRAY_SLOT_ACCESSOR(ModuleObject, importEntries, ImportEntriesSlot)
DEFINE_ARRAY_SLOT_ACCESSOR(ModuleObject, localExportEntries, LocalExportEntriesSlot)
DEFINE_ARRAY_SLOT_ACCESSOR(ModuleObject, indirectExportEntries, IndirectExportEntriesSlot)
DEFINE_ARRAY_SLOT_ACCESSOR(ModuleObject, starExportEntries, StarExportEntriesSlot)

/* static */ bool
ModuleObject::isInstance(HandleValue value)
{
    return value.isObject() && value.toObject().is<ModuleObject>();
}

/* static */ ModuleObject*
ModuleObject::create(JSContext* cx)
{
    RootedObject proto(cx, cx->global()->getModulePrototype());
    RootedObject obj(cx, NewObjectWithGivenProto(cx, &class_, proto));
    if (!obj)
        return nullptr;

    RootedModuleObject self(cx, &obj->as<ModuleObject>());

    Zone* zone = cx->zone();
    IndirectBindingMap* bindings = zone->new_<IndirectBindingMap>(zone);
    if (!bindings || !bindings->init()) {
        ReportOutOfMemory(cx);
        js_delete<IndirectBindingMap>(bindings);
        return nullptr;
    }

    self->initReservedSlot(ImportBindingsSlot, PrivateValue(bindings));

    FunctionDeclarationVector* funDecls = zone->new_<FunctionDeclarationVector>(zone);
    if (!funDecls) {
        ReportOutOfMemory(cx);
        return nullptr;
    }

    self->initReservedSlot(FunctionDeclarationsSlot, PrivateValue(funDecls));
    return self;
}

/* static */ void
ModuleObject::finalize(js::FreeOp* fop, JSObject* obj)
{
    MOZ_ASSERT(fop->maybeOnHelperThread());
    ModuleObject* self = &obj->as<ModuleObject>();
    if (self->hasImportBindings())
        fop->delete_(&self->importBindings());
    if (IndirectBindingMap* bindings = self->namespaceBindings())
        fop->delete_(bindings);
    if (FunctionDeclarationVector* funDecls = self->functionDeclarations())
        fop->delete_(funDecls);
}

ModuleEnvironmentObject*
ModuleObject::environment() const
{
    MOZ_ASSERT(status() != MODULE_STATUS_ERRORED);

    Value value = getReservedSlot(EnvironmentSlot);
    if (value.isUndefined())
        return nullptr;

    return &value.toObject().as<ModuleEnvironmentObject>();
}

bool
ModuleObject::hasImportBindings() const
{
    // Import bindings may not be present if we hit OOM in initialization.
    return !getReservedSlot(ImportBindingsSlot).isUndefined();
}

IndirectBindingMap&
ModuleObject::importBindings()
{
    return *static_cast<IndirectBindingMap*>(getReservedSlot(ImportBindingsSlot).toPrivate());
}

JSObject*
ModuleObject::namespaceExports()
{
    Value value = getReservedSlot(NamespaceExportsSlot);
    if (value.isUndefined())
        return nullptr;

    return &value.toObject();
}

IndirectBindingMap*
ModuleObject::namespaceBindings()
{
    Value value = getReservedSlot(NamespaceBindingsSlot);
    if (value.isUndefined())
        return nullptr;

    return static_cast<IndirectBindingMap*>(value.toPrivate());
}

ModuleNamespaceObject*
ModuleObject::namespace_()
{
    Value value = getReservedSlot(NamespaceSlot);
    if (value.isUndefined())
        return nullptr;
    return &value.toObject().as<ModuleNamespaceObject>();
}

FunctionDeclarationVector*
ModuleObject::functionDeclarations()
{
    Value value = getReservedSlot(FunctionDeclarationsSlot);
    if (value.isUndefined())
        return nullptr;

    return static_cast<FunctionDeclarationVector*>(value.toPrivate());
}

void
ModuleObject::init(HandleScript script)
{
    initReservedSlot(ScriptSlot, PrivateValue(script));
    initReservedSlot(StatusSlot, Int32Value(MODULE_STATUS_ERRORED));
}

void
ModuleObject::setInitialEnvironment(HandleModuleEnvironmentObject initialEnvironment)
{
    initReservedSlot(InitialEnvironmentSlot, ObjectValue(*initialEnvironment));
}

void
ModuleObject::initImportExportData(HandleArrayObject requestedModules,
                                   HandleArrayObject importEntries,
                                   HandleArrayObject localExportEntries,
                                   HandleArrayObject indirectExportEntries,
                                   HandleArrayObject starExportEntries)
{
    initReservedSlot(RequestedModulesSlot, ObjectValue(*requestedModules));
    initReservedSlot(ImportEntriesSlot, ObjectValue(*importEntries));
    initReservedSlot(LocalExportEntriesSlot, ObjectValue(*localExportEntries));
    initReservedSlot(IndirectExportEntriesSlot, ObjectValue(*indirectExportEntries));
    initReservedSlot(StarExportEntriesSlot, ObjectValue(*starExportEntries));
    setReservedSlot(StatusSlot, Int32Value(MODULE_STATUS_UNINSTANTIATED));
}

static bool
FreezeObjectProperty(JSContext* cx, HandleNativeObject obj, uint32_t slot)
{
    RootedObject property(cx, &obj->getSlot(slot).toObject());
    return FreezeObject(cx, property);
}

/* static */ bool
ModuleObject::Freeze(JSContext* cx, HandleModuleObject self)
{
    return FreezeObjectProperty(cx, self, RequestedModulesSlot) &&
           FreezeObjectProperty(cx, self, ImportEntriesSlot) &&
           FreezeObjectProperty(cx, self, LocalExportEntriesSlot) &&
           FreezeObjectProperty(cx, self, IndirectExportEntriesSlot) &&
           FreezeObjectProperty(cx, self, StarExportEntriesSlot) &&
           FreezeObject(cx, self);
}

#ifdef DEBUG

static inline bool
CheckObjectFrozen(JSContext* cx, HandleObject obj, bool* result)
{
    return TestIntegrityLevel(cx, obj, IntegrityLevel::Frozen, result);
}

static inline bool
CheckObjectPropertyFrozen(JSContext* cx, HandleNativeObject obj, uint32_t slot, bool* result)
{
    RootedObject property(cx, &obj->getSlot(slot).toObject());
    return CheckObjectFrozen(cx, property, result);
}

/* static */ inline bool
ModuleObject::AssertFrozen(JSContext* cx, HandleModuleObject self)
{
    static const mozilla::EnumSet<ModuleSlot> slotsToCheck = {
        RequestedModulesSlot,
        ImportEntriesSlot,
        LocalExportEntriesSlot,
        IndirectExportEntriesSlot,
        StarExportEntriesSlot
    };

    bool frozen = false;
    for (auto slot : slotsToCheck) {
        if (!CheckObjectPropertyFrozen(cx, self, slot, &frozen))
            return false;
        MOZ_ASSERT(frozen);
    }

    if (!CheckObjectFrozen(cx, self, &frozen))
        return false;
    MOZ_ASSERT(frozen);

    return true;
}

#endif

inline static void
AssertModuleScopesMatch(ModuleObject* module)
{
    MOZ_ASSERT(module->enclosingScope()->is<GlobalScope>());
    MOZ_ASSERT(IsGlobalLexicalEnvironment(&module->initialEnvironment().enclosingEnvironment()));
}

void
ModuleObject::fixEnvironmentsAfterCompartmentMerge()
{
    AssertModuleScopesMatch(this);
    initialEnvironment().fixEnclosingEnvironmentAfterCompartmentMerge(script()->global());
    AssertModuleScopesMatch(this);
}

bool
ModuleObject::hasScript() const
{
    // When modules are parsed via the Reflect.parse() API, the module object
    // doesn't have a script.
    return !getReservedSlot(ScriptSlot).isUndefined();
}

JSScript*
ModuleObject::script() const
{
    return static_cast<JSScript*>(getReservedSlot(ScriptSlot).toPrivate());
}

static inline void
AssertValidModuleStatus(ModuleStatus status)
{
    MOZ_ASSERT(status >= MODULE_STATUS_ERRORED && status <= MODULE_STATUS_EVALUATED);
}

ModuleStatus
ModuleObject::status() const
{
    ModuleStatus status = getReservedSlot(StatusSlot).toInt32();
    AssertValidModuleStatus(status);
    return status;
}

Value
ModuleObject::error() const
{
    MOZ_ASSERT(status() == MODULE_STATUS_ERRORED);
    return getReservedSlot(ErrorSlot);
}

Value
ModuleObject::hostDefinedField() const
{
    return getReservedSlot(HostDefinedSlot);
}

void
ModuleObject::setHostDefinedField(const JS::Value& value)
{
    setReservedSlot(HostDefinedSlot, value);
}

ModuleEnvironmentObject&
ModuleObject::initialEnvironment() const
{
    return getReservedSlot(InitialEnvironmentSlot).toObject().as<ModuleEnvironmentObject>();
}

Scope*
ModuleObject::enclosingScope() const
{
    return script()->enclosingScope();
}

/* static */ void
ModuleObject::trace(JSTracer* trc, JSObject* obj)
{
    ModuleObject& module = obj->as<ModuleObject>();
    if (module.hasScript()) {
        JSScript* script = module.script();
        TraceManuallyBarrieredEdge(trc, &script, "Module script");
        module.setReservedSlot(ScriptSlot, PrivateValue(script));
    }

    if (module.hasImportBindings())
        module.importBindings().trace(trc);
    if (IndirectBindingMap* bindings = module.namespaceBindings())
        bindings->trace(trc);

    if (FunctionDeclarationVector* funDecls = module.functionDeclarations())
        funDecls->trace(trc);
}

void
ModuleObject::createEnvironment()
{
    // The environment has already been created, we just neet to set it in the
    // right slot.
    MOZ_ASSERT(!getReservedSlot(InitialEnvironmentSlot).isUndefined());
    MOZ_ASSERT(getReservedSlot(EnvironmentSlot).isUndefined());
    setReservedSlot(EnvironmentSlot, getReservedSlot(InitialEnvironmentSlot));
}

bool
ModuleObject::noteFunctionDeclaration(JSContext* cx, HandleAtom name, HandleFunction fun)
{
    FunctionDeclarationVector* funDecls = functionDeclarations();
    if (!funDecls->emplaceBack(name, fun)) {
        ReportOutOfMemory(cx);
        return false;
    }

    return true;
}

/* static */ bool
ModuleObject::instantiateFunctionDeclarations(JSContext* cx, HandleModuleObject self)
{
#ifdef DEBUG
    if (!AssertFrozen(cx, self))
        return false;
#endif

    FunctionDeclarationVector* funDecls = self->functionDeclarations();
    if (!funDecls) {
        JS_ReportErrorASCII(cx, "Module function declarations have already been instantiated");
        return false;
    }

    RootedModuleEnvironmentObject env(cx, &self->initialEnvironment());
    RootedFunction fun(cx);
    RootedObject obj(cx);
    RootedValue value(cx);

    for (const auto& funDecl : *funDecls) {
        fun = funDecl.fun;
        obj = Lambda(cx, fun, env);
        if (!obj)
            return false;

        if (fun->isAsync()) {
            if (fun->isGenerator())
                obj = WrapAsyncGenerator(cx, obj.as<JSFunction>());
            else
                obj = WrapAsyncFunction(cx, obj.as<JSFunction>());
        }

        if (!obj)
            return false;

        value = ObjectValue(*obj);
        if (!SetProperty(cx, env, funDecl.name->asPropertyName(), value))
            return false;
    }

    js_delete(funDecls);
    self->setReservedSlot(FunctionDeclarationsSlot, UndefinedValue());
    return true;
}

/* static */ bool
ModuleObject::execute(JSContext* cx, HandleModuleObject self, MutableHandleValue rval)
{
#ifdef DEBUG
    if (!AssertFrozen(cx, self))
        return false;
#endif

    RootedScript script(cx, self->script());
    RootedModuleEnvironmentObject scope(cx, self->environment());
    if (!scope) {
        JS_ReportErrorASCII(cx, "Module declarations have not yet been instantiated");
        return false;
    }

    return Execute(cx, script, *scope, rval.address());
}

/* static */ ModuleNamespaceObject*
ModuleObject::createNamespace(JSContext* cx, HandleModuleObject self, HandleObject exports)
{
    MOZ_ASSERT(!self->namespace_());
    MOZ_ASSERT(exports->is<ArrayObject>());

    RootedModuleNamespaceObject ns(cx, ModuleNamespaceObject::create(cx, self));
    if (!ns)
        return nullptr;

    Zone* zone = cx->zone();
    IndirectBindingMap* bindings = zone->new_<IndirectBindingMap>(zone);
    if (!bindings || !bindings->init()) {
        ReportOutOfMemory(cx);
        js_delete<IndirectBindingMap>(bindings);
        return nullptr;
    }

    self->initReservedSlot(NamespaceSlot, ObjectValue(*ns));
    self->initReservedSlot(NamespaceExportsSlot, ObjectValue(*exports));
    self->initReservedSlot(NamespaceBindingsSlot, PrivateValue(bindings));
    return ns;
}

static bool
InvokeSelfHostedMethod(JSContext* cx, HandleModuleObject self, HandlePropertyName name)
{
    RootedValue fval(cx);
    if (!GlobalObject::getSelfHostedFunction(cx, cx->global(), name, name, 0, &fval))
        return false;

    RootedValue ignored(cx);
    return Call(cx, fval, self, &ignored);
}

/* static */ bool
ModuleObject::Instantiate(JSContext* cx, HandleModuleObject self)
{
    return InvokeSelfHostedMethod(cx, self, cx->names().ModuleInstantiate);
}

/* static */ bool
ModuleObject::Evaluate(JSContext* cx, HandleModuleObject self)
{
    return InvokeSelfHostedMethod(cx, self, cx->names().ModuleEvaluate);
}

DEFINE_GETTER_FUNCTIONS(ModuleObject, namespace_, NamespaceSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, status, StatusSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, error, ErrorSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, requestedModules, RequestedModulesSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, importEntries, ImportEntriesSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, localExportEntries, LocalExportEntriesSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, indirectExportEntries, IndirectExportEntriesSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, starExportEntries, StarExportEntriesSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, dfsIndex, DFSIndexSlot)
DEFINE_GETTER_FUNCTIONS(ModuleObject, dfsAncestorIndex, DFSAncestorIndexSlot)

/* static */ bool
GlobalObject::initModuleProto(JSContext* cx, Handle<GlobalObject*> global)
{
    static const JSPropertySpec protoAccessors[] = {
        JS_PSG("namespace", ModuleObject_namespace_Getter, 0),
        JS_PSG("status", ModuleObject_statusGetter, 0),
        JS_PSG("error", ModuleObject_errorGetter, 0),
        JS_PSG("requestedModules", ModuleObject_requestedModulesGetter, 0),
        JS_PSG("importEntries", ModuleObject_importEntriesGetter, 0),
        JS_PSG("localExportEntries", ModuleObject_localExportEntriesGetter, 0),
        JS_PSG("indirectExportEntries", ModuleObject_indirectExportEntriesGetter, 0),
        JS_PSG("starExportEntries", ModuleObject_starExportEntriesGetter, 0),
        JS_PSG("dfsIndex", ModuleObject_dfsIndexGetter, 0),
        JS_PSG("dfsAncestorIndex", ModuleObject_dfsAncestorIndexGetter, 0),
        JS_PS_END
    };

    static const JSFunctionSpec protoFunctions[] = {
        JS_SELF_HOSTED_FN("getExportedNames", "ModuleGetExportedNames", 1, 0),
        JS_SELF_HOSTED_FN("resolveExport", "ModuleResolveExport", 2, 0),
        JS_SELF_HOSTED_FN("declarationInstantiation", "ModuleInstantiate", 0, 0),
        JS_SELF_HOSTED_FN("evaluation", "ModuleEvaluate", 0, 0),
        JS_FS_END
    };

    RootedObject proto(cx, GlobalObject::createBlankPrototype<PlainObject>(cx, global));
    if (!proto)
        return false;

    if (!DefinePropertiesAndFunctions(cx, proto, protoAccessors, protoFunctions))
        return false;

    global->setReservedSlot(MODULE_PROTO, ObjectValue(*proto));
    return true;
}

#undef DEFINE_GETTER_FUNCTIONS
#undef DEFINE_STRING_ACCESSOR_METHOD
#undef DEFINE_ARRAY_SLOT_ACCESSOR

///////////////////////////////////////////////////////////////////////////
// ModuleBuilder

ModuleBuilder::ModuleBuilder(JSContext* cx, HandleModuleObject module,
                             const frontend::TokenStream& tokenStream)
  : cx_(cx),
    module_(cx, module),
    tokenStream_(tokenStream),
    requestedModuleSpecifiers_(cx, AtomSet(cx)),
    requestedModules_(cx, RequestedModuleVector(cx)),
    importedBoundNames_(cx, AtomVector(cx)),
    importEntries_(cx, ImportEntryVector(cx)),
    exportEntries_(cx, ExportEntryVector(cx)),
    localExportEntries_(cx, ExportEntryVector(cx)),
    indirectExportEntries_(cx, ExportEntryVector(cx)),
    starExportEntries_(cx, ExportEntryVector(cx))
{}

bool
ModuleBuilder::init()
{
    return requestedModuleSpecifiers_.init();
}

bool
ModuleBuilder::buildTables()
{
    for (const auto& e : exportEntries_) {
        RootedExportEntryObject exp(cx_, e);
        if (!exp->moduleRequest()) {
            RootedImportEntryObject importEntry(cx_, importEntryFor(exp->localName()));
            if (!importEntry) {
                if (!localExportEntries_.append(exp))
                    return false;
            } else {
                if (importEntry->importName() == cx_->names().star) {
                    if (!localExportEntries_.append(exp))
                        return false;
                } else {
                    MOZ_ASSERT(exp->lineNumber());
                    RootedAtom exportName(cx_, exp->exportName());
                    RootedAtom moduleRequest(cx_, importEntry->moduleRequest());
                    RootedAtom importName(cx_, importEntry->importName());
                    RootedExportEntryObject exportEntry(cx_);
                    exportEntry = ExportEntryObject::create(cx_,
                                                            exportName,
                                                            moduleRequest,
                                                            importName,
                                                            nullptr,
                                                            exp->lineNumber(),
                                                            exp->columnNumber());
                    if (!exportEntry || !indirectExportEntries_.append(exportEntry))
                        return false;
                }
            }
        } else if (exp->importName() == cx_->names().star) {
            if (!starExportEntries_.append(exp))
                return false;
        } else {
            MOZ_ASSERT(exp->lineNumber());
            if (!indirectExportEntries_.append(exp))
                return false;
        }
    }

    return true;
}

bool
ModuleBuilder::initModule()
{
    RootedArrayObject requestedModules(cx_, createArray(requestedModules_));
    if (!requestedModules)
        return false;

    RootedArrayObject importEntries(cx_, createArray(importEntries_));
    if (!importEntries)
        return false;

    RootedArrayObject localExportEntries(cx_, createArray(localExportEntries_));
    if (!localExportEntries)
        return false;

    RootedArrayObject indirectExportEntries(cx_, createArray(indirectExportEntries_));
    if (!indirectExportEntries)
        return false;

    RootedArrayObject starExportEntries(cx_, createArray(starExportEntries_));
    if (!starExportEntries)
        return false;

    module_->initImportExportData(requestedModules,
                                 importEntries,
                                 localExportEntries,
                                 indirectExportEntries,
                                 starExportEntries);

    return true;
}

bool
ModuleBuilder::processImport(frontend::ParseNode* pn)
{
    MOZ_ASSERT(pn->isKind(PNK_IMPORT));
    MOZ_ASSERT(pn->isArity(PN_BINARY));
    MOZ_ASSERT(pn->pn_left->isKind(PNK_IMPORT_SPEC_LIST));
    MOZ_ASSERT(pn->pn_right->isKind(PNK_STRING));

    RootedAtom module(cx_, pn->pn_right->pn_atom);
    if (!maybeAppendRequestedModule(module, pn->pn_right))
        return false;

    for (ParseNode* spec = pn->pn_left->pn_head; spec; spec = spec->pn_next) {
        MOZ_ASSERT(spec->isKind(PNK_IMPORT_SPEC));
        MOZ_ASSERT(spec->pn_left->isArity(PN_NAME));
        MOZ_ASSERT(spec->pn_right->isArity(PN_NAME));

        RootedAtom importName(cx_, spec->pn_left->pn_atom);
        RootedAtom localName(cx_, spec->pn_right->pn_atom);

        if (!importedBoundNames_.append(localName))
            return false;

        uint32_t line;
        uint32_t column;
        tokenStream_.lineNumAndColumnIndex(spec->pn_left->pn_pos.begin, &line, &column);

        RootedImportEntryObject importEntry(cx_);
        importEntry = ImportEntryObject::create(cx_, module, importName, localName, line, column);
        if (!importEntry || !importEntries_.append(importEntry))
            return false;
    }

    return true;
}

bool
ModuleBuilder::processExport(frontend::ParseNode* pn)
{
    MOZ_ASSERT(pn->isKind(PNK_EXPORT) || pn->isKind(PNK_EXPORT_DEFAULT));
    MOZ_ASSERT(pn->getArity() == (pn->isKind(PNK_EXPORT) ? PN_UNARY : PN_BINARY));

    bool isDefault = pn->getKind() == PNK_EXPORT_DEFAULT;
    ParseNode* kid = isDefault ? pn->pn_left : pn->pn_kid;

    if (isDefault && pn->pn_right) {
        // This is an export default containing an expression.
        RootedAtom localName(cx_, cx_->names().starDefaultStar);
        RootedAtom exportName(cx_, cx_->names().default_);
        return appendExportEntry(exportName, localName);
    }

    switch (kid->getKind()) {
      case PNK_EXPORT_SPEC_LIST:
        MOZ_ASSERT(!isDefault);
        for (ParseNode* spec = kid->pn_head; spec; spec = spec->pn_next) {
            MOZ_ASSERT(spec->isKind(PNK_EXPORT_SPEC));
            RootedAtom localName(cx_, spec->pn_left->pn_atom);
            RootedAtom exportName(cx_, spec->pn_right->pn_atom);
            if (!appendExportEntry(exportName, localName, spec))
                return false;
        }
        break;

      case PNK_CLASS: {
        const ClassNode& cls = kid->as<ClassNode>();
        MOZ_ASSERT(cls.names());
        RootedAtom localName(cx_, cls.names()->innerBinding()->pn_atom);
        RootedAtom exportName(cx_, isDefault ? cx_->names().default_ : localName.get());
        if (!appendExportEntry(exportName, localName))
            return false;
        break;
      }

      case PNK_VAR:
      case PNK_CONST:
      case PNK_LET: {
        MOZ_ASSERT(kid->isArity(PN_LIST));
        for (ParseNode* var = kid->pn_head; var; var = var->pn_next) {
            if (var->isKind(PNK_ASSIGN))
                var = var->pn_left;
            MOZ_ASSERT(var->isKind(PNK_NAME));
            RootedAtom localName(cx_, var->pn_atom);
            RootedAtom exportName(cx_, isDefault ? cx_->names().default_ : localName.get());
            if (!appendExportEntry(exportName, localName))
                return false;
        }
        break;
      }

      case PNK_FUNCTION: {
        RootedFunction func(cx_, kid->pn_funbox->function());
        MOZ_ASSERT(!func->isArrow());
        RootedAtom localName(cx_, func->explicitName());
        RootedAtom exportName(cx_, isDefault ? cx_->names().default_ : localName.get());
        MOZ_ASSERT_IF(isDefault, localName);
        if (!appendExportEntry(exportName, localName))
            return false;
        break;
      }

      default:
        MOZ_CRASH("Unexpected parse node");
    }

    return true;
}

bool
ModuleBuilder::processExportFrom(frontend::ParseNode* pn)
{
    MOZ_ASSERT(pn->isKind(PNK_EXPORT_FROM));
    MOZ_ASSERT(pn->isArity(PN_BINARY));
    MOZ_ASSERT(pn->pn_left->isKind(PNK_EXPORT_SPEC_LIST));
    MOZ_ASSERT(pn->pn_right->isKind(PNK_STRING));

    RootedAtom module(cx_, pn->pn_right->pn_atom);
    if (!maybeAppendRequestedModule(module, pn->pn_right))
        return false;

    for (ParseNode* spec = pn->pn_left->pn_head; spec; spec = spec->pn_next) {
        if (spec->isKind(PNK_EXPORT_SPEC)) {
            RootedAtom bindingName(cx_, spec->pn_left->pn_atom);
            RootedAtom exportName(cx_, spec->pn_right->pn_atom);
            if (!appendExportFromEntry(exportName, module, bindingName, spec->pn_left))
                return false;
        } else {
            MOZ_ASSERT(spec->isKind(PNK_EXPORT_BATCH_SPEC));
            RootedAtom importName(cx_, cx_->names().star);
            if (!appendExportFromEntry(nullptr, module, importName, spec))
                return false;
        }
    }

    return true;
}

ImportEntryObject*
ModuleBuilder::importEntryFor(JSAtom* localName) const
{
    for (auto import : importEntries_) {
        if (import->localName() == localName)
            return import;
    }
    return nullptr;
}

bool
ModuleBuilder::hasExportedName(JSAtom* name) const
{
    for (auto entry : exportEntries_) {
        if (entry->exportName() == name)
            return true;
    }
    return false;
}

bool
ModuleBuilder::appendExportEntry(HandleAtom exportName, HandleAtom localName, ParseNode* node)
{
    uint32_t line = 0;
    uint32_t column = 0;
    if (node)
        tokenStream_.lineNumAndColumnIndex(node->pn_pos.begin, &line, &column);

    Rooted<ExportEntryObject*> exportEntry(cx_);
    exportEntry = ExportEntryObject::create(cx_, exportName, nullptr, nullptr, localName,
                                            line, column);
    return exportEntry && exportEntries_.append(exportEntry);
}

bool
ModuleBuilder::appendExportFromEntry(HandleAtom exportName, HandleAtom moduleRequest,
                                     HandleAtom importName, ParseNode* node)
{
    uint32_t line;
    uint32_t column;
    tokenStream_.lineNumAndColumnIndex(node->pn_pos.begin, &line, &column);

    Rooted<ExportEntryObject*> exportEntry(cx_);
    exportEntry = ExportEntryObject::create(cx_, exportName, moduleRequest, importName, nullptr,
                                            line, column);
    return exportEntry && exportEntries_.append(exportEntry);
}

bool
ModuleBuilder::maybeAppendRequestedModule(HandleAtom specifier, ParseNode* node)
{
    if (requestedModuleSpecifiers_.has(specifier))
        return true;

    uint32_t line;
    uint32_t column;
    tokenStream_.lineNumAndColumnIndex(node->pn_pos.begin, &line, &column);

    JSContext* cx = cx_;
    RootedRequestedModuleObject req(cx, RequestedModuleObject::create(cx, specifier, line, column));
    if (!req)
        return false;

    return FreezeObject(cx, req) &&
           requestedModules_.append(req) &&
           requestedModuleSpecifiers_.put(specifier);
}

template <typename T>
ArrayObject* ModuleBuilder::createArray(const JS::Rooted<GCVector<T>>& vector)
{
    uint32_t length = vector.length();
    RootedArrayObject array(cx_, NewDenseFullyAllocatedArray(cx_, length));
    if (!array)
        return nullptr;

    array->setDenseInitializedLength(length);
    for (uint32_t i = 0; i < length; i++)
        array->initDenseElement(i, ObjectValue(*vector[i]));

    return array;
}
