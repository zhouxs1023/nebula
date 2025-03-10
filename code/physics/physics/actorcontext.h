#pragma once
//------------------------------------------------------------------------------
/**
    Diverse functions for manipulating physics actors

    (C) 2019 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "physicsinterface.h"
#include "ids/idgenerationpool.h"
#include "math/vector.h"
#include "math/plane.h"
#include "math/matrix44.h"

namespace Physics
{

class PhysxState;
class StreamActorPool;

class ActorContext
{
public:
    /// helper functions for creating shapes
   ///
    static ActorId CreateBox(Math::vector const& extends, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    static ActorId CreateSphere(float radius, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);
    ///
    static ActorId CreatePlane(Math::plane const& plane, IndexT material, IndexT scene = 0);
    ///
    static ActorId CreateCapsule(float radius, float halfHeight, IndexT material, bool dynamic, Math::matrix44 const & transform, IndexT scene = 0);

    ///
    static Actor& GetActor(ActorId id);
    ///
    static void SetTransform(ActorId id, Math::matrix44 const & transform);
    ///
    static Math::matrix44 GetTransform(ActorId id);

    ///
    static void SetLinearVelocity(ActorId id, Math::vector speed);
    ///
    static Math::vector GetLinearVelocity(ActorId id);

    ///
    static void SetAngularVelocity(ActorId id, Math::vector speed);
    ///
    static Math::vector GetAngularVelocity(ActorId id);

    /// apply a global impulse vector at the next time step at a global position
    void ApplyImpulseAtPos(ActorId id, const Math::vector& impulse, const Math::point& pos);


    /// shortcut for getting the pxactor object
    static physx::PxRigidActor* GetPxActor(ActorId id);
    /// shortcut for getting the pxactor object
    static physx::PxRigidDynamic* GetPxDynamic(ActorId id);

    static bool IsValid(ActorId id);

    friend class PhysxState;
    friend class StreamActorPool;
private:
    static Util::Array<Actor> actors;
    static Ids::IdGenerationPool actorPool;
    ///
    static ActorId AllocateActorId(physx::PxRigidActor* pxActor);
    ///
    static ActorId AllocateActorId(physx::PxRigidActor* pxActor, ActorResourceId res);
    ///
    static void DiscardActor(ActorId id);
};



//------------------------------------------------------------------------------
/**
*/
inline bool
ActorContext::IsValid(ActorId id)
{
    return ActorContext::actorPool.IsValid(id.id);
}

}