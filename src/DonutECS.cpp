// Module includes
#include "DonutECS.h"

namespace ECS
{
    EntityManager::EntityManager()
    {
        for (Entity i = 0; i < MAX_ENTITIES; ++i)
            availableEntities.push(i);
    }

    Entity EntityManager::createEntity()
    {
        assert(livingEntityCount < MAX_ENTITIES && "Too many entities!");

        if (availableEntities.empty())
            return INVALID_ENTITY;

        Entity id = availableEntities.front();
        availableEntities.pop();
        ++livingEntityCount;
        signatures[id].reset();
        return id;
    }

    void EntityManager::destroyEntity(Entity entity)
    {
        assert(entity < MAX_ENTITIES && "Entity out of range.");
        if (entity >= MAX_ENTITIES) return;

        signatures[entity].reset();
        availableEntities.push(entity);
        if (livingEntityCount > 0) --livingEntityCount;
    }

    void EntityManager::setSignature(Entity entity, Signature signature)
    {
        assert(entity < MAX_ENTITIES);
        signatures[entity] = signature;
    }

    Signature EntityManager::getSignature(Entity entity) const
    {
        assert(entity < MAX_ENTITIES);
        return signatures[entity];
    }

    // SystemManager
    void SystemManager::entityDestroyed(Entity entity)
    {
        for (auto& pair : systems)
        {
            auto& ents = pair.second->entities;
            ents.erase(std::remove(ents.begin(), ents.end(), entity), ents.end());
        }
    }

    void SystemManager::entitySignatureChanged(Entity entity, Signature entitySignature)
    {
        for (auto& pair : systems)
        {
            auto const& typeName = pair.first;
            auto& system = pair.second;
            auto const& sysSig = signatures[typeName];

            if ((entitySignature & sysSig) == sysSig)
            {
                if (std::find(system->entities.begin(), system->entities.end(), entity) == system->entities.end())
                    system->entities.push_back(entity);
            }
            else
            {
                auto& ents = system->entities;
                ents.erase(std::remove(ents.begin(), ents.end(), entity), ents.end());
            }
        }
    }

    void Coordinator::destroyEntity(Entity entity)
    {
        entityManager->destroyEntity(entity);
        componentManager->entityDestroyed(entity);
        systemManager->entityDestroyed(entity);
    }
}