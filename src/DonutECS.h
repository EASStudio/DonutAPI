#pragma once

#define DONUTECS_VERSION "1.0"

// Core includes
#include <unordered_map>
#include <array>
#include <bitset>
#include <queue>
#include <vector>
#include <string>
#include <cstdint>
#include <typeindex>
#include <memory>
#include <cassert>
#include <algorithm>  
#include <utility>

namespace ECS
{
    // Config

    using Entity = std::uint32_t;
    constexpr Entity MAX_ENTITIES = 10000;
    constexpr Entity INVALID_ENTITY = 0xFFFFFFFF;

    using ComponentType = std::uint8_t;
    constexpr ComponentType MAX_COMPONENTS = 64;

    using Signature = std::bitset<MAX_COMPONENTS>;

    // Classes

    class Coordinator;

    class EntityManager
    {
    public:
        EntityManager();

        Entity createEntity();
        void destroyEntity(Entity entity);

        void setSignature(Entity entity, Signature signature);
        Signature getSignature(Entity entity) const;

    private:
        std::queue<Entity> availableEntities;
        std::array<Signature, MAX_ENTITIES> signatures{};
        uint32_t livingEntityCount = 0;
    };

    class System
    {
    public:
        virtual ~System() = default;
        virtual void update(float dt) {}

        std::vector<Entity> entities;
        Coordinator* coordinator = nullptr;
    };

    class SystemManager
    {
    public:
        template<typename T>
        std::shared_ptr<T> registerSystem()
        {
            std::type_index typeName = typeid(T);
            assert(systems.find(typeName) == systems.end() && "System already registered!");

            auto system = std::make_shared<T>();
            systems[typeName] = system;
            return system;
        }

        template<typename T>
        void setSignature(Signature signature)
        {
            std::type_index typeName = typeid(T);
            assert(systems.find(typeName) != systems.end() && "System not registered!");
            signatures[typeName] = signature;
        }

        void entityDestroyed(Entity entity);
        void entitySignatureChanged(Entity entity, Signature entitySignature);

    private:
        std::unordered_map<std::type_index, Signature> signatures;
        std::unordered_map<std::type_index, std::shared_ptr<System>> systems;
    };

    class IComponentArray
    {
    public:
        virtual ~IComponentArray() = default;
        virtual void entityDestroyed(Entity entity) = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray
    {
    public:
        void insert(Entity entity, T component)
        {
            assert(size < MAX_ENTITIES && "Too many components!");
            size_t newIndex = size;
            entityToIndex[entity] = newIndex;
            indexToEntity[newIndex] = entity;
            components[newIndex] = std::move(component);
            ++size;
        }

        void remove(Entity entity)
        {
            auto it = entityToIndex.find(entity);
            assert(it != entityToIndex.end() && "Removing non-existent component.");

            size_t indexOfRemoved = it->second;
            size_t indexOfLast = size - 1;

            // Move last element into removed slot (cache-friendly)
            components[indexOfRemoved] = std::move(components[indexOfLast]);

            Entity lastEntity = indexToEntity[indexOfLast];
            entityToIndex[lastEntity] = indexOfRemoved;
            indexToEntity[indexOfRemoved] = lastEntity;

            entityToIndex.erase(entity);
            --size;
        }

        T& get(Entity entity)
        {
            auto it = entityToIndex.find(entity);
            assert(it != entityToIndex.end() && "Component not found!");
            return components[it->second];
        }

        const T& get(Entity entity) const
        {
            auto it = entityToIndex.find(entity);
            assert(it != entityToIndex.end() && "Component not found!");
            return components[it->second];
        }

        bool hasComponent(Entity entity) const
        {
            return entityToIndex.find(entity) != entityToIndex.end();
        }

        void entityDestroyed(Entity entity) override
        {
            if (hasComponent(entity))
                remove(entity);
        }

    private:
        std::array<T, MAX_ENTITIES> components{};
        std::unordered_map<Entity, size_t> entityToIndex;
        std::array<Entity, MAX_ENTITIES> indexToEntity{};
        size_t size = 0;
    };

    class ComponentManager
    {
    public:
        template<typename T>
        void registerComponent()
        {
            std::type_index typeName = typeid(T);
            assert(componentTypes.find(typeName) == componentTypes.end() && "Component already registered!");
            assert(nextComponentType < MAX_COMPONENTS && "Too many component types - raise MAX_COMPONENTS.");

            componentTypes[typeName] = nextComponentType;
            componentArrays[typeName] = std::make_shared<ComponentArray<T>>();
            ++nextComponentType;
        }

        template<typename T>
        ComponentType getComponentType() const
        {
            std::type_index typeName = typeid(T);
            auto it = componentTypes.find(typeName);
            assert(it != componentTypes.end() && "Component not registered!");
            return it->second;
        }

        template<typename T>
        void addComponent(Entity entity, T component)
        {
            getComponentArray<T>()->insert(entity, std::move(component));
        }

        template<typename T>
        void removeComponent(Entity entity)
        {
            getComponentArray<T>()->remove(entity);
        }

        template<typename T>
        T& getComponent(Entity entity)
        {
            return getComponentArray<T>()->get(entity);
        }

        template<typename T>
        const T& getComponent(Entity entity) const
        {
            return getComponentArray<T>()->get(entity);
        }

        template<typename T>
        bool hasComponent(Entity entity) const
        {
            return getComponentArray<T>()->hasComponent(entity);
        }

        void entityDestroyed(Entity entity)
        {
            for (auto& pair : componentArrays)
                pair.second->entityDestroyed(entity);
        }

    private:
        template<typename T>
        std::shared_ptr<ComponentArray<T>> getComponentArray() const
        {
            std::type_index typeName = typeid(T);
            auto it = componentArrays.find(typeName);
            assert(it != componentArrays.end() && "Component not registered!");
            return std::static_pointer_cast<ComponentArray<T>>(it->second);
        }

        std::unordered_map<std::type_index, ComponentType> componentTypes;
        std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays;
        ComponentType nextComponentType = 0;
    };

    class Coordinator
    {
    public:
        // Constructed ready to use. init() still works and simply starts over -
        // previously every method dereferenced a null unique_ptr until it was called.
        Coordinator() { init(); }

        void init()
        {
            entityManager = std::make_unique<EntityManager>();
            componentManager = std::make_unique<ComponentManager>();
            systemManager = std::make_unique<SystemManager>();
        }

        Entity createEntity() { return entityManager->createEntity(); }
        void destroyEntity(Entity entity);

        // Component API
        template<typename T> void registerComponent() { componentManager->registerComponent<T>(); }
        template<typename T> ComponentType getComponentType() const { return componentManager->getComponentType<T>(); }

        template<typename T>
        void addComponent(Entity entity, T component)
        {
            componentManager->addComponent<T>(entity, std::move(component));

            Signature signature = entityManager->getSignature(entity);
            signature.set(getComponentType<T>(), true);
            entityManager->setSignature(entity, signature);

            systemManager->entitySignatureChanged(entity, signature);
        }

        template<typename T>
        void removeComponent(Entity entity)
        {
            componentManager->removeComponent<T>(entity);

            Signature signature = entityManager->getSignature(entity);
            signature.set(getComponentType<T>(), false);
            entityManager->setSignature(entity, signature);

            systemManager->entitySignatureChanged(entity, signature);
        }

        template<typename T> T& getComponent(Entity entity) { return componentManager->getComponent<T>(entity); }
        template<typename T> const T& getComponent(Entity entity) const { return componentManager->getComponent<T>(entity); }
        template<typename T> bool hasComponent(Entity entity) const { return componentManager->hasComponent<T>(entity); }

        // System API
        template<typename T>
        std::shared_ptr<T> registerSystem()
        {
            // System::coordinator was declared but never wired up, so systems had no
            // way back to the coordinator that owns them.
            std::shared_ptr<T> system = systemManager->registerSystem<T>();
            if (system) system->coordinator = this;
            return system;
        }

        template<typename T>
        void setSystemSignature(Signature signature)
        {
            systemManager->setSignature<T>(signature);
        }

    private:
        std::unique_ptr<EntityManager> entityManager;
        std::unique_ptr<ComponentManager> componentManager;
        std::unique_ptr<SystemManager> systemManager;
    };
}