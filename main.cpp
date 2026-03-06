#include "reflect.hpp"   // IWYU pragma: keep
#include <string>
#include <vector>

class Person final {
public:
    std::string family;
    bool isFemale = false;
    // NOLINTNEXTLINE
    void IntroduceMyself() const volatile&& noexcept {
        puts("This is IntroduceMyself().");
    }
    [[nodiscard]] bool getFemale() const {
        puts("This is isFemale().");
        return isFemale;
    }
    bool getMarried(Person& other) {
        bool success = other.isFemale != isFemale;
        family = (success) ? " Mrs." + other.family : "Mr." + other.family;
        puts("Married!");
        return success;
    }
};
enum Color : uint8_t {
    red,
    blue,
    green,
};
enum class Permission : uint8_t {
    read = 1,
    write = 2,
    excute = 4,
    all = 7,
};

namespace reflect::Dynamic {
namespace factory {
template<typename T> class Enum;
}   // namespace factory
class Numberic;
class Type {
public:
    static std::map<std::string_view, const Type*> list;

    enum class Kind : uint8_t { Numberic, Enum, Class, Unkown };
    virtual ~Type() = default;
    explicit Type(Kind kind, std::string_view name)
        : _kind(kind)
        , _name{name} {}
    [[nodiscard]] const std::string& getName() const { return _name; }
    [[nodiscard]] const Kind& getKind() const { return _kind; }
#if __cpp_concepts
    template<typename T>
        requires std::is_base_of_v<Type, T>
#else
    template<typename T,
             typename = std::enable_if_t<std::is_base_of_v<Type, T>>>
#endif
    [[nodiscard]] const T* as() const {
        return static_cast<const T*>(this);
    }

private:
    template<typename T> friend class factory::Enum;
    Kind _kind;
    std::string _name;
};
class Numberic final : public Type {
public:
    enum class Kind : uint8_t {
        Unknown = 0,
        Int8 = 1,
        Int16 = 2,
        Int32 = 4,
        Int64 = 8,
        Float = 16,
        Double = 32,
        Long_Double = 64
    };
    Numberic(Kind kind, bool is_signed)
        : Type{Type::Kind::Numberic, to_string(kind)}
        , _kind{kind}
        , _is_signed(is_signed) {
        Type::list[getName()] = static_cast<Type*>(this);
    }

#if __cpp_concepts
    template<typename T>
        requires std::is_arithmetic_v<T>
#else
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
#endif
    static Numberic create() {
        return Numberic{to_Kind<T>(), std::is_signed_v<T>};
    }
    [[nodiscard]] Kind getKind() const { return _kind; }
    [[nodiscard]] bool getSigned() const { return _is_signed; }

private:
    Kind _kind;
    bool _is_signed;
    static std::string to_string(Kind kind) {
        switch (kind) {
            case Kind::Int8:        return "int8";
            case Kind::Int16:       return "Int16";
            case Kind::Int32:       return "int32";
            case Kind::Int64:       return "int64";
            case Kind::Float:       return "float";
            case Kind::Double:      return "double";
            case Kind::Long_Double: return "long double";
            default:                return "Unknown";
        }
    }
#if __cpp_concepts
    template<typename T>
        requires std::is_arithmetic_v<T>
#else
    template<typename T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
#endif
    static Kind to_Kind() {
        return std::is_integral_v<T> ? static_cast<Kind>(sizeof(T))
                                     : static_cast<Kind>(sizeof(T) * 4);
    }
};
class Enum final : public Type {
private:
    using value_type = int32_t;
    using enum_map = std::map<std::string_view, value_type>;
    enum_map _map;

public:
    explicit Enum()
        : Type{Kind::Unkown, "Unknown-Enum"} {
        Type::list[getName()] = static_cast<Type*>(this);
    }
    explicit Enum(std::string_view name)
        : Type{Kind::Enum, name} {}
    template<typename T> void add(std::string_view name, T value) {
        _map[name] = static_cast<value_type>(value);
        Type::list[getName()] = static_cast<Type*>(this);
    }
    [[nodiscard]] const enum_map& getMap() const { return _map; }
};
struct MemVar {
    std::string _name;
    const Type* type;
};
struct MemFunc {
    std::string _name;
    const Type* _retType;
    std::vector<const Type*> paramTypes;
};

class Class final : public Type {
public:
    explicit Class(const std::string& name)
        : Type(Type::Kind::Class, name) {}

private:
    std::vector<MemVar> _vars;
    std::vector<MemFunc> _fns;
};
namespace factory {

template<typename T> class Numberic final {
public:
    static Numberic& Instance() {
        static Numberic inst{reflect::Dynamic::Numberic::create<T>()};
        return inst;
    }
    [[nodiscard]] const reflect::Dynamic::Numberic& Info() const {
        return _info;
    }

private:
    reflect::Dynamic::Numberic _info;
    explicit Numberic(reflect::Dynamic::Numberic&& info)
        : _info{std::move(info)} {}
};
template<typename T> class Enum final {
public:
    static Enum& Instance() {
        static Enum inst;
        return inst;
    }
    [[nodiscard]] const reflect::Dynamic::Enum& Info() const { return _info; }
    Enum& Regist(std::string_view name) {
        _info._name = name;
        return *this;
    };
    template<typename U> Enum& add(std::string_view name, U value) {
        _info.add(name, value);
        return *this;
    }
    void UnRegist() { _info = reflect::Dynamic::Enum{}; }

private:
    reflect::Dynamic::Enum _info;
};
class Trivial final {
public:
    static Trivial& Instance() {
        static Trivial inst;
        return inst;
    }
};
template<typename T> class factory final {
public:
    static auto& getFactory() {
        if constexpr (std::is_arithmetic_v<T>)
            return Numberic<T>::Instance();
        else if constexpr (std::is_enum_v<T>)
            return Enum<T>::Instance();
        else if constexpr (std::is_class_v<T>)
            return Enum<T>::Instance();
        else
            return Trivial::Instance();
    }
};
}   // namespace factory
template<typename T> auto& Register() {
    return factory::factory<T>::getFactory();
}
template<typename T> const Type* getType() {
    return &Register<T>().Info();
}
const Type* getType(std::string_view type) {
    return Type::list.find(type)->second;
}
std::map<std::string_view, const Type*> Type::list{};
}   // namespace reflect::Dynamic

constexpr const char* strbool(bool value) {
    return value ? "true" : "false";
}

[[maybe_unused]] constexpr const char _template_isFamle[]{"isFemale"};
[[maybe_unused]] constexpr const char _template_getFamle[]{"getFamle"};

RFS_OBJ_BEGIN(Person)
    RFS_OBJ_MEM(isFemale)
    // static constexpr auto _isFemale =
    //     SRefl ::field_traits<decltype(&class_t::isFemale)>{
    //         &class_t ::isFemale, "isFemale"};
    RFS_OBJ_MEM_TEM(getFemale, _template_getFamle)
RFS_OBJ_END()

RFS_ENUM_BEGIN(Color)
    RFS_ENUM_LIST({MAKE_NOR_FIELD_TRAITS(red)}, {MAKE_NOR_FIELD_TRAITS(blue)})
RFS_ENUM_END()

RFS_ENUM_BEGIN(Permission)
    RFS_ENUM_LIST({MAKE_FIELD_TRAITS(NOR, Permission::read)},
                  {MAKE_FIELD_TRAITS(NOR, Permission::write)},
                  {MAKE_FIELD_TRAITS(NOR, Permission::all)})
RFS_ENUM_END()

int main() {
    using ClassInfo = SRefl::TypeInfo<Person>;
    Person man;
    printf("Type: %s\n", ClassInfo::_name.data());
    puts("Member:");
    printf("\tMember Data: \n");
    printf("\t\t%s: %s\n", ClassInfo::Registry::_isFemale.getName().data(),
           strbool(ClassInfo::Registry::_isFemale._ptr));
    printf("\tMember Function: \n");
    printf("\t\t%s:\n", ClassInfo::Registry::_getFemale.from_TempName().data());
    printf("\t\tFuntion running:\n");
    printf("\t\t\t");
    printf("\t\tFuntion retval: %s\n", strbool((man.*ClassInfo::Registry::_getFemale._ptr)()));

    printf("\n****************************************\n");

    using ColorInfo = SRefl::TypeInfo<Color>;
    auto& ColorList = ColorInfo::list;
    printf("Type: %s\n", ColorInfo::_name.data());
    puts("Member:");
    printf("\tData: \n");
    // printf("\t\t%s: %s\n", ColorList[Color::red].data(),
    //        ColorInfo[Color::red]);
    for (auto list : ColorInfo::list)
        printf("\t\t%s: %d\n", list.second.data(), list.first);
    return 0;
}
