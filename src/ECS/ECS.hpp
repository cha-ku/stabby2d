#ifndef ECS_HPP
#define ECS_HPP

#include <cstdint>
#include <bitset>
#include <vector>
#include <unordered_map>
#include <set>
#include <typeindex>
#include <memory>
#include "../Logger/Logger.hpp"

constexpr uint8_t MAX_COMPONENTS = 128;
// Each System needs to keep track of what components are active for that particular system at a point
// This is done using a bitset as a map for component IDs. If in the bitset position i is set it means Component with ID i is set for a System
using Signature = std::bitset<MAX_COMPONENTS>;

class IComponent {
protected:
  inline static unsigned int nextId;
};

template <typename T>
class Component : public IComponent {
public:
  static unsigned int GetId() {
    static const auto componentId = nextId++;
    return componentId;
  }
};

class Entity {
private:
    uint64_t id;

public:
    // // Rule of five (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five)
    explicit Entity(uint64_t entityId) : id(entityId) {}
    Entity(const Entity& entity)=default;
    Entity(Entity&& entity)=default;
    Entity& operator=(const Entity& entity) = default;
    Entity& operator=(Entity&& entity) = default;
    ~Entity()=default;

    [[nodiscard]] unsigned int GetId() const;

    bool operator==(const Entity& other) const { return id == other.id; }
    bool operator!=(const Entity& other) const { return id != other.id; }
    bool operator<(const Entity& other) const { return id < other.id; }
    bool operator>(const Entity& other) const { return id > other.id; }
};

// System processes specific entities
class System {
private:
   Signature componentSignature;
   std::vector<Entity> entities;

public:
   void AddEntity(const Entity& entity);
   void RemoveEntity(Entity& entity);
   [[nodiscard]] std::vector<Entity> GetEntities() const;
   [[nodiscard]] Signature const& GetComponentSignature() const;

   // Valid entities must have atleast one component
   template <typename TComponent> void RequireComponent();
};

// IPool is pure virtual base class
class IPool {
public:
    virtual ~IPool() {} // virtual destructor
};

// Pool is a container to store components
template <typename T>
class Pool : public IPool {
private:
  mutable std::vector<T> data;

public:
  // Rule of five (https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rc-five)
  explicit Pool(uint16_t size = 64) {
      data.resize(size);
  };

  Pool(Pool& other) : data{other.data} {}

  Pool& operator=(const Pool& other) = default;

  Pool(Pool&& other) noexcept : Pool(other) {}

  Pool& operator=(Pool&& other) noexcept = default;

  ~Pool() override = default;

  auto IsEmpty() const {
      return data.empty();
  }

  // @brief Get size of underlying storage
  // @return size_t
  auto Size() const -> size_t {
      return data.size();
  }

  // @brief vector::reserve if n is greater than underlying container size, vector::resize otherwise
  // @param n
  // @return void
  auto Resize(size_t n) const -> void { data.resize(n); }

  auto Clear() -> void { data.clear(); }

  auto Add(T& object) -> void {
    data.emplace_back(object);
  }

  auto Set(size_t index, T& object) -> void {
      data[index] = object;
  }

  auto Get(size_t index) -> T& {
      return static_cast<T&>(data[index]);
  }

  T& operator [](size_t index) {
      return data[static_cast<int>(index)];
  }
};

// registry class is responsible for creating, removing and tracking entities, components and systems
class Registry {
private:
  mutable size_t numEntities = 0;

  // component IDs are sequential and each element in componentPools points to a Pool of Entities associated with that particular Component.
  mutable std::vector<std::shared_ptr<IPool>> componentPools;

  // tracks which component is turned 'on' (i.e. Signature) per entity.
  mutable std::vector<Signature> entityComponentSignatures;

  // track each system type with map
  std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

  // only add/delete entities at the end of game loop
  mutable std::set<Entity> entitiesToBeAdded;
  mutable std::set<Entity> entitiesToBeKilled;

public:
  // Entity management
  Entity CreateEntity();

  // Component management
  template<typename TComponent, typename ...TArgs> void AddComponent(Entity& entity, TArgs&& ...args);
  template<typename TComponent> void RemoveComponent(Entity& entity);
  template<typename TComponent> bool HasComponent(Entity& entity);

  // System management
  template<typename TSystem, typename ...TArgs> void AddSystem(TArgs&& ...args);
  template<typename TSystem> void RemoveSystem();
  template<typename TSystem> bool HasSystem();
  template<typename TSystem> const TSystem& GetSystem();

  // If an entity's component signature matches to that of a system's required components,
  // add that entity to the system
  void AddEntityToSystems(const Entity& entity);
  void Update();
};

template<typename TComponent>
void System::RequireComponent() {
  const auto componentId = Component<TComponent>::GetId();
  componentSignature.set(componentId);
};

template<typename TComponent, typename... TArgs>
inline void Registry:: AddComponent(Entity &entity, TArgs &&...args) {
  const auto componentId = Component<TComponent>::GetId();
  const auto entityId = entity.GetId();

  // resize if component ID is greater than what componentPools can hold
  if (componentId >= componentPools.size()) {
    componentPools.resize(componentId + 1, nullptr);
  }

  // create Pool for a Component type if it doesn't exist
  if (!componentPools[componentId]) {
    componentPools[componentId] = std::make_shared<Pool<TComponent>>();
  }

  std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);

  if (entityId >= componentPool->Size()) {
    componentPool->Resize(numEntities);
  }

  TComponent newComponent(std::forward<TArgs>(args)...);

  componentPool->Set(entityId, newComponent);

  entityComponentSignatures.resize(componentPools.size());
  Logger::Info("entityComponentSignatures size " + std::to_string(entityComponentSignatures.size()));
  entityComponentSignatures[entityId].set(componentId);
  Logger::Info("Component ID " + std::to_string(componentId) + " added to entity ID " + std::to_string(entityId));
};

template<typename TComponent>
inline void Registry::RemoveComponent(Entity &entity) {
  auto const componentId = Component<TComponent>::GetId();
  auto const entityId = entity.GetId();

  entityComponentSignatures[entityId].set(componentId, false);
};

template<typename TComponent>
inline bool Registry::HasComponent(Entity &entity) {
  auto const componentId = Component<TComponent>::GetId();
  auto const entityId = entity.GetId();

  return entityComponentSignatures[entityId].test(componentId);
};

template<typename TSystem, typename... TArgs>
inline void Registry::AddSystem(TArgs&& ...args) {
  systems[std::type_index(typeid(TSystem))] = std::make_shared<TSystem>(TSystem(std::forward<TArgs>(args)...));
};

template<typename TSystem>
inline void Registry::RemoveSystem(){
  std::erase_if(systems, [](const auto& item) {
      return item.first == std::type_index(typeid(TSystem));
  });
};

template<typename TSystem>
inline bool Registry::HasSystem() {
    return systems.count(std::type_index(typeid(TSystem)));
};

template<typename TSystem>
inline const TSystem& Registry::GetSystem() {
    return std::ref(*systems[std::type_index(typeid(TSystem))]);
};

#endif
