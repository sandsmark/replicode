/*
 *  MemImpl.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 23/12/2009.
 *
 */

#ifndef __MEM_IMPL_H
#define __MEM_IMPL_H

#include "hash_containers"
#include "Mem.h"
#include "Core.h"
#include "Object.h"
#include "Group.h"
#include "utils.h"
#include <map>

using namespace std;

namespace r_exec {

namespace MemImpl {

	struct ObjectBase;
	struct ObjectImpl;
	struct ViewImpl;
	struct GroupImpl;
	struct Impl;
	
	struct MediatedValueAccumulator {
		MediatedValueAccumulator() :requestCount(0), sumTargetValue(0) {}
		float getAndReset();
		int requestCount;
		float sumTargetValue;
	};
		
	struct ControlValue {
		float value;
		float valueAtLastChangePeriod;
		bool lowValueNotified;
		bool highValueNotified;
	};

	struct ViewImpl {
		ObjectBase* object;
		GroupImpl* group;
		GroupImpl* originGroup;
		int originNodeID;
		int64 injectionTime;
		ControlValue saliency;
		float saliencyDecayPerPeriod;
		int saliencyPeriodsOfDecayRemaining;
		ControlValue resilience;
		ControlValue activationOrVisibility; // for reactive objects and groups
		float copyOnVisibility; // for groups; not mediated
		
		// These 'is*' flags lag the actual status of the view: they are
		// updated during the same update cycle, but after the control values are changed.
		// isSalient, in particular, is not modified until the very end
		// of the update, to allow the code to accurately detect when views
		// cross the boundary between non-saliency and saliency in the context
		// of visibility from other groups.
		bool isExisting;
		bool isSalient;
		bool isActive;
		bool isVisible;
		
		MediatedValueAccumulator mediations[3];
		
		struct object_hash {
			size_t operator() (ViewImpl* view) const { return reinterpret_cast<size_t>(view->object); }
		};
		struct equal_object {
			bool operator()(const ViewImpl* a, const ViewImpl* b) const { return a->object == b->object; }
		};
	};
	
	struct ObjectBase : public Object
	{
		ObjectBase();
		virtual ~ObjectBase();
		void retain(const char* msg);
		void release(const char* msg);
		Expression copyMarkerSet(ReductionInstance& dest) const;
		Expression copyViewSet(ReductionInstance& dest) const;
		Expression copyVisibleView(ReductionInstance& dest, const Group* group) const;
		Expression copyLocalView(ReductionInstance& dest, const Group* group) const;

		Expression copyViewInternal(ReductionInstance& dest, const ViewImpl* view) const;
		void prepareForCopy(ReductionInstance& dest) const;
		virtual void debug();
		
		typedef UNORDERED_MAP<const Group*, ViewImpl*> ViewStore;
		typedef UNORDERED_SET<ObjectImpl*> MarkerStore;
		int refCount;
		MarkerStore markers;
		ViewStore views;
		float propagationSaliencyThreshold;
		enum {REACTIVE, GROUP, OBJECT} type;
		bool isNotification;
	};
	
	struct ObjectImpl : public ObjectBase
	{
		~ObjectImpl();
		ObjectImpl() :hash_value(0) {}
		Expression copy(ReductionInstance& dest) const;
		Object* getReference(int index) const;
		const Group* asGroup() const { return 0; }
		void debug();

		std::vector<ObjectBase*> references;
		std::vector<r_code::Atom> atoms;
		
		size_t hash_value;
		struct hash { size_t operator() (const ObjectImpl* obj) const; };
		struct equal_object { bool operator()(const ObjectImpl* a, const ObjectImpl* b) const; };
	};
	
	struct GroupImpl : public ObjectBase, public Group
	{
		~GroupImpl();
		GroupImpl(std::vector<r_code::Atom>& atoms, std::vector<Object*>& references);
		Expression copy(ReductionInstance& dest) const;
		Object* getReference(int index) const;
		const Group* asGroup() const;
		Object* asObject() { return this; }
		void receive(ReductionInstance* instance);
		
		// methods used during the update
		void processCommands();
			void generateReductionNotification(ReductionInstance* ri);
			void processInjectionOrEjection(ReductionInstance* ri, Expression command);
			void processModOrSet(ReductionInstance* ri, Expression command);
		void updateControlValues(bool updateGeneral, int64 resilienceDecrease);
			void processNotifications(
				ControlValue& value,
				int32 updateCounter,
				float32 changeThreshold,
				int32 changePeriod,
				float32 lowValueThreshold,
				float32 highValueThreshold,
				int32 valuePeriod,
				int16 changeMarkerOpcode,
				int16 lowValueMarkerOpcode,
				int16 highValueMarkerOpcode,
				ViewImpl* view
			);
				void addNotificationMarker(ObjectImpl* obj, ViewImpl* view);
		void processNewViews();
		void processStatusChangesAndUpdateStatistics();
			void processStatusChangeAndUpdateStatistics(ViewImpl* view);
				void makeGroupVisible(ViewImpl* view);
		void propagateVisibleViews();
		void presentNewlySalientObjects();
		void cleanup();

		void debug();
		// There are two methods of accessing the values of the group: by
		// name and by index.  We need read-write access by index (read for
		// implementing copy(), write for implementing mod/set), and read
		// access by name.
		// here, values[0] == saliencyThreshold,
		//       values[1] == activationThreshold,
		// etc.
		
		// WARNING! When making changes to this structure, examine:
		//   GroupImpl::copy
		//   GroupImpl::GroupImpl
		//   GroupImpl::processModOrSet
		// to see if the assumptions made about the ordering of values
		//  are still correct.
		union {
			float values[1];
			struct {
				float updatePeriod;
				float signalingPeriod;
				float saliencyThreshold; // mediated
				float activationThreshold; // mediated
				float visibilityThreshold; // mediated
				float cSaliency; // mediated
				float cSaliencyThreshold; // mediated
				float cActivation; // mediated
				float cActivationThreshold; // mediated
				float saliencyDecayPercentage;
				float saliencyDecayTarget;
				float saliencyDecayPeriod;
				float saliencyChangeNotificationThreshold;
				float saliencyChangeNotificationPeriod;
				float activationChangeNotificationThreshold;
				float activationChangeNotificationPeriod;
				float saliencyAverageValue;
				float saliencyHighValue;
				float saliencyLowValue;
				float activationAverageValue;
				float activationHighValue;
				float activationLowValue;
				float saliencyHighValueThreshold;
				float saliencyLowValueThreshold;
				float saliencyValueNotificationPeriod;
				float activationHighValueThreshold;
				float activationLowValueThreshold;
				float activationValueNotificationPeriod;
				float resilienceHighValueThreshold;
				float resilienceLowValueThreshold;
				float resilienceValueNotificationPeriod;
				
				float lastValueSentinel;
			};
		};
		
		// exposed member
		GroupImpl* notificationGroup;
		
		// non-exposed members
		typedef UNORDERED_MAP<const ObjectBase*, ViewImpl*> ContentStore;
		typedef UNORDERED_SET<ViewImpl*, ViewImpl::object_hash, ViewImpl::equal_object> SalientStore;
		typedef multimap<int64, ViewImpl*> PendingViewStore;
		
		Core::Instance* coreInstance;
		Impl* mem;
		MediatedValueAccumulator mediations[7];
		ContentStore content;
		PendingViewStore pendingViews;
		std::vector<ReductionInstance*> reductions;
		std::vector<ViewImpl*> newContent;
		std::vector<ViewImpl*> insertedContent;
		std::vector<ViewImpl*> localNewlySalientContent;
		SalientStore combinedNewlySalientContent;
		
		int32 numGeneralUpdatesSkipped;
		int32 numSignalingPeriodsSkipped;
		int32 updateCounter;
		bool controlValuesChanged;
		float32 sumSaliency;
		float32 sumActivation;
		int32 numObjects;
		int32 numReactiveObjects;
	};
	
	struct Impl : public Mem
	{
		// implementation of Mem interface
		Impl(ObjectReceiver *receiver_, int64 resilienceUpdatePeriod_, int64 baseUpdatePeriod_, std::vector<r_code::Object*> objects);
		~Impl();
		void receive(
			Object *object,
			std::vector<r_code::Atom> viewData,
			int sourceNodeId,
			ObjectReceiver::Destination dest
		);
		void beginBatchReceive();
		void endBatchReceive();
		void resume();
		void suspend();
		void update();
		void processInsertions();
		ObjectBase* insertObject(ObjectBase* obj);
			
		void receiveInternal(
			Object* object, std::vector<r_code::Atom> viewData,
			int sourceNodeId, GroupImpl* dest
		);
		void debug();
		Mutex insertionQueueMutex;
		bool insideBatchReceive;
		struct InsertionQueueEntry {
			Object* object;
			std::vector<r_code::Atom> viewData;
			int node_id;
			GroupImpl* destination;
		};
		typedef std::vector<InsertionQueueEntry> InsertionStore;
		InsertionStore insertionQueue;

		typedef UNORDERED_MAP<const Group*, GroupImpl*> GroupStore;
		typedef UNORDERED_SET<ObjectImpl*, ObjectImpl::hash, ObjectImpl::equal_object> ObjectStore;
		
		Thread *runThread;
		static thread_ret thread_function_call run(void* args);
		ObjectReceiver *output;
		Core* core;
		GroupImpl* rootGroup;
		GroupImpl* stdinGroup;
		GroupImpl* stdoutGroup;
		ObjectImpl* self;
		GroupStore groups;
		ObjectStore objects;
		
		int64 resilienceUpdatePeriod;
		int64 baseUpdatePeriod;
		
		int64 nextResilienceUpdate;
		int64 nextGeneralUpdate;
	};
}

}

#endif // __MEM_IMPL_H
