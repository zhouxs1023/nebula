#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::Message

	Messages the main communications channel between components.

	Messages aren't exclusive to component however, any method or function can be
	used as a callback from a message which means you can hook into a message
	from anywhere. This can be useful for debugging, tools, managers etc.

	A component can register a message callback by using the __RegisterMsg macro.

	Messages should be generated using Nebula's IDLC.
	If that's not preferred, you can implement a message by deriving from the
	Game::Message class and overriding the GetName, GetFourCC, Send and Defer methods.
	This class uses curiously recurring template patterns and you need to provide the
	template with the subclass as the first template argument. The rest of the
	template arguments are the callbacks parameters.

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "ids/idgenerationpool.h"
#include "util/delegate.h"
#include "util/hashtable.h"
#include "util/arrayallocator.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "util/fourcc.h"

#define __DeclareMsg(NAME, FOURCC, ...) \
class NAME : public Game::Message<NAME, __VA_ARGS__> \
{ \
	public: \
		NAME() = delete; \
		~NAME() = delete; \
		\
		constexpr static const char* GetName() \
		{ \
			return #NAME; \
		}; \
		\
		constexpr static const uint GetFourCC() \
		{ \
			return FOURCC; \
		}; \
};

#define __RegisterMsg(MSGTYPE, FUNCTION) \
	MSGTYPE::Register(MSGTYPE::Delegate::FromFunction<FUNCTION>())

#define __this_RegisterMsg(MSGTYPE, METHOD) \
	MSGTYPE::Register(MSGTYPE::Delegate::FromMethod<std::remove_pointer<decltype(this)>::type, &std::remove_pointer<decltype(this)>::type::METHOD>(this))

/// Removes const reference from T.
template<class T>
using UnqualifiedType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

namespace Game
{

ID_32_TYPE(MessageListenerId)

//------------------------------------------------------------------------------
/**
*/
struct MessageListener
{
	Util::FourCC messageId;
	MessageListenerId listenerId;
};

//------------------------------------------------------------------------------
/**
*/
template <class MSG, class RETURNTYPE, class ... TYPES>
class Fetcher
{
public:
	Fetcher<MSG, RETURNTYPE, TYPES...>(const Fetcher<MSG, RETURNTYPE, TYPES...>&) = delete;
	void operator=(const Fetcher<MSG, RETURNTYPE, TYPES...>&) = delete;

	/// callback type.
	typedef RETURNTYPE(*callback_function)(TYPES...);

	/// Register a listener to this message. There can be only one listener to this type of message.
	/// Will return a listener that must be used to deregister.
	static MessageListener Register(callback_function func)
	{
		n_assert2(Instance()->callback == nullptr, "There can be only one listener to a fetch message");
		Instance()->callback = func;
		MessageListener listener = { Instance()->fourcc, Instance()->validId };
		return listener;
	}

	/// Deregister a listener
	static void Deregister(MessageListener listener)
	{
		n_assert2(listener.fourcc == Instance()->fourcc, "MessageListener does not belong to this message!");
		n_assert2(listener.listenerId == validId, "MessageListener is not valid. It might already have been deregistered!");
		Instance()->callback = nullptr;
		Instance()->validId++;
	}

	/// Fetch a value. Returns the returnvalue of the callback.
	static RETURNTYPE Fetch(TYPES... values)
	{
		return Instance()->callback(values ...);
	}
private:
	friend MSG;

	Fetcher() :
		callback(nullptr),
		validId(0)
	{
		this->name = MSG::GetName();
		this->fourcc = MSG::GetFourCC();
	}

	~Fetcher()
	{
		// empty
	}

	MessageListenerId validId;

	// There can be only ONE!
	callback_function callback;

	Util::StringAtom name;
	Util::FourCC fourcc;

	static Fetcher<MSG, RETURNTYPE, TYPES...>* Instance()
	{
		static Fetcher<MSG, RETURNTYPE, TYPES...> instance;
		return &instance;
	}
};

//------------------------------------------------------------------------------
/**
	@todo	Biggest performance hog right now is that listeners
			themselves need to check if, for example, an entity is
			registered or not. Although this is potentially O(1) per lookup,
			it can probably be improved by adding some registry over which
			entities are registered to which component systems.
			This will however add a lot of memory overhead that is not
			necessarily needed for anything else.
			In the end, the api will (hopefully) most likely to stay the same.
*/
template <class MSG, class ... TYPES>
class Message
{
public:
	ID_32_TYPE(MessageQueueId)

	Message();
	~Message();

	Message<MSG, TYPES...>(const Message<MSG, TYPES...>&) = delete;
	void operator=(const Message<MSG, TYPES...>&) = delete;

	/// Type definition for this message's delegate
	using Delegate = Util::Delegate<void(TYPES...)>;

	/// Register a listener to this message. Returns an ID for the listener so that we can associate it.
	static MessageListener Register(Delegate&& callback);

	/// Deregister a listener
	static void Deregister(MessageListener listener);

	/// Send a message
	static void Send(TYPES ... values);

	/// Creates a new message queue for deferred dispatching
	static MessageQueueId AllocateMessageQueue();

	/// Add a message to a message queue
	static void Defer(MessageQueueId qid, TYPES ... values);
	
	/// Dispatch all messages in a message queue
	static void DispatchMessageQueue(MessageQueueId id);

	/// Deallocate a message queue. All messages in the queue will be destroyed.
	static void DeAllocateMessageQueue(MessageQueueId id);

	/// Check whether a message queue is still valid.
	static bool IsMessageQueueValid(MessageQueueId id);

	/// Send a network distributed message
	static void Distribute(TYPES ...);

	/// Deregisters all listeners at once.
	static void DeregisterAll();

	/// Returns whether a MessageListenerId is still registered or not.
	static bool IsValid(MessageListenerId listener);

protected:
	friend MSG;

	static Message<MSG, TYPES...>* Instance()
	{
		static Message<MSG, TYPES...> instance;
		return &instance;
	}

	/// Internal singleton instance
	
	/// Registry between component and index in list.
	Util::HashTable<MessageListenerId, IndexT> listenerMap;

	/// contains the callback and the listener it's attached to.
	Util::ArrayAllocator<
		MessageListenerId,
		Delegate
	> callbacks;

	/// id generation pool for the deferred messages queues.
	Ids::IdGenerationPool messageQueueIdPool;

	/// Deferred messages
	Util::Array<Util::ArrayAllocator<UnqualifiedType<TYPES> ...>> messageQueues;

	/// Contains the arguments of a recieved distributed message.
	/// @todo	This should be updated by some NetworkMessageManager-y object.
	Util::ArrayAllocator<
		UnqualifiedType<TYPES> ...
	> distributedMessages;

protected:
	Util::StringAtom name;
	Util::FourCC fourcc;
	Ids::IdGenerationPool listenerPool;


private:
	template<std::size_t...Is>
	void send_expander(Util::ArrayAllocator<UnqualifiedType<TYPES> ...>& data, const SizeT& index, std::index_sequence<Is...>)
	{
		Send(data.Get<Is>(index)...);
	}

	void send_expander(Util::ArrayAllocator<UnqualifiedType<TYPES>...>& data, const SizeT& index)
	{
		this->send_expander(data, index, std::make_index_sequence<sizeof...(TYPES)>());
	}
};

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline
Message<MSG, TYPES...>::Message()
{
	this->name = MSG::GetName();
	this->fourcc = MSG::GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline
Message<MSG, TYPES...>::~Message()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline MessageListener
Message<MSG, TYPES...>::Register(Delegate&& callback)
{
	MessageListenerId l;
	Instance()->listenerPool.Allocate(l.id);
	IndexT index = Instance()->callbacks.Alloc();
	Instance()->callbacks.Set(index, l, callback);
	Instance()->listenerMap.Add(l, index);
	MessageListener listener = { Instance()->fourcc, l };
	return listener;
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::Deregister(MessageListener listener)
{
	auto instance = Instance();
	n_assert(listener.messageId == instance->fourcc);
	IndexT index = instance->listenerMap[listener.listenerId];
	if (index != InvalidIndex)
	{
		instance->listenerMap.Erase(listener.listenerId);
		instance->callbacks.EraseIndexSwap(index);
		if (instance->callbacks.Size() != 0)
		{
			instance->listenerMap[instance->callbacks.Get<0>(index)] = index;
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::Send(TYPES ... values)
{
	auto instance = Instance();
	SizeT size = instance->callbacks.Size();
	for (SizeT i = 0; i < size; ++i)
	{
		// n_assert(instance->callbacks.Get<1>(i).GetObject() != nullptr);
		instance->callbacks.Get<1>(i)(values...);
	}
}

//------------------------------------------------------------------------------
/**
*/
template<typename MSG, class ...TYPES>
inline typename Message<MSG, TYPES...>::MessageQueueId
Message<MSG, TYPES...>::AllocateMessageQueue()
{
	auto instance = Instance();
	MessageQueueId id;
	instance->messageQueueIdPool.Allocate(id.id);
	
	if (Ids::Index(id.id) >= instance->messageQueues.Size())
	{
		// This should garantuee that the id is the newest element
		n_assert(instance->messageQueues.Size() == Ids::Index(id.id));
		instance->messageQueues.Append({});
	}
	
	return id;
}

template<typename MSG, class ...TYPES>
inline void 
Message<MSG, TYPES...>::Defer(MessageQueueId qid, TYPES ...values)
{
	auto instance = Instance();
	SizeT index = Ids::Index(qid.id);

	auto i = instance->messageQueues[index].Alloc();
	instance->messageQueues[index].Set(i, values...);
}

//------------------------------------------------------------------------------
/**
*/
template<typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::DispatchMessageQueue(MessageQueueId id)
{
	auto instance = Instance();
	
	n_assert(instance->messageQueues.Size() > Ids::Index(id.id));
	n_assert(instance->messageQueueIdPool.IsValid(id.id));

	auto& data = instance->messageQueues[Ids::Index(id.id)];

	SizeT size = data.Size();
	for (SizeT i = 0; i < size; i++)
	{
		instance->send_expander(data, i);
	}

	// TODO:	Should we call reset here instead?
	data.Clear();
}

//------------------------------------------------------------------------------
/**
*/
template<typename MSG, class ...TYPES> inline void
Message<MSG, TYPES...>::DeAllocateMessageQueue(MessageQueueId id)
{
	auto instance = Instance();
	
	n_assert(instance->messageQueues.Size() > Ids::Index(id.id));
	n_assert(instance->messageQueueIdPool.IsValid(id.id));

	instance->messageQueues[Ids::Index(id.id)].Clear();
	instance->messageQueueIdPool.Deallocate(id.id);
}

template<typename MSG, class ...TYPES> inline bool
Message<MSG, TYPES...>::IsMessageQueueValid(MessageQueueId id)
{
	return Instance()->messageQueueIdPool.IsValid(id.id);
}



//------------------------------------------------------------------------------
/**
	@todo	Implement networked messages.
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::Distribute(TYPES ...)
{
	n_error("Network distributed messages not yet supported!");
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline void
Message<MSG, TYPES...>::DeregisterAll()
{
	auto instance = Instance();
	SizeT size = instance->callbacks.Size();
	for (SizeT i = 0; i < size; i++)
	{
		instance->listenerPool.Deallocate(instance->callbacks.Get<0>(i).id);
	}
	instance->callbacks.Clear();
	instance->distributedMessages.Clear();
	instance->listenerMap.Clear();	
}

//------------------------------------------------------------------------------
/**
*/
template <typename MSG, class ... TYPES>
inline bool
Message<MSG, TYPES...>::IsValid(MessageListenerId listener)
{
	return Instance()->listenerPool.IsValid(listener.id);
}

} // namespace Msg
