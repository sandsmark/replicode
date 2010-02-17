#include "../r_code/types.h"
#include "MemImpl.h"
#include "Expression.h"
#include "mbrane_imports/utils.h"
#include "opcodes.h"
#include <math.h>
#include <stdio.h>

using namespace std;
using r_code::Atom;
namespace r_exec { namespace MemImpl {


	Impl::Impl(ObjectReceiver *output_, vector<r_code::Object*> objects)
		:output(output_)
	{
		core = Core::create();
		inputGroup = new GroupImpl(core, this);
		outputGroup = new GroupImpl(core, this);
		groups[inputGroup] = inputGroup;
		groups[outputGroup] = outputGroup;
		
		int64 now = mBrane::Time::Get();
		nextResilienceUpdate = now + resilienceUpdatePeriod;
		nextGeneralUpdate = now + baseUpdatePeriod;
		runThread = mBrane::Thread::New<mBrane::Thread>(Impl::run, this);
	}

	Impl::~Impl()
	{
		
	}

	void Impl::receive(
		Object *object,
		std::vector<Atom> viewData,
		int node_id,
		Destination dest
	) {
		receiveInternal(object, viewData, node_id,
			dest == ObjectReceiver::INPUT_GROUP ? inputGroup : outputGroup);
	}
	
	void Impl::receiveInternal(
		Object* object, std::vector<Atom> viewData,
		int node_id, GroupImpl* dest
	) {
		InsertionQueueEntry e;
		e.object = object;
		e.viewData = viewData;
		e.node_id = node_id;
		e.destination = dest;
		if (!insideBatchReceive) insertionQueueMutex.acquire();
			insertionQueue.push_back(e);
		if (!insideBatchReceive) insertionQueueMutex.release();
	}
	
	void Impl::beginBatchReceive()
	{
		insertionQueueMutex.acquire();
		insideBatchReceive = true;
	}
	
	void Impl::endBatchReceive()
	{
		insideBatchReceive = false;
		insertionQueueMutex.release();
	}

	thread_ret thread_function_call Impl::run(void* args)
	{
		Impl* this_ = reinterpret_cast<Impl*>(args);
		for (;;) {
			this_->runThread->Sleep(500);
			this_->update();
		}
	}

	void Impl::resume()
	{
		printf("memimpl resuming\n");
		runThread->resume();
	}

	void Impl::suspend()
	{
		printf("memimpl suspending\n");
		runThread->suspend();
	}

	void Impl::update()
	{
		printf("memimpl update\n");
		
		core->suspend();
		
		processInsertions();
		GroupStore::iterator it;
		for (it = groups.begin(); it != groups.end(); ++it) {
			it->second->processCommands();
		}
		
		bool updateGeneral = false;
		int64 resilienceDecrease = 0;
		int64 now = mBrane::Time::Get();
		if (now >= nextGeneralUpdate) {
			updateGeneral = true;
			nextGeneralUpdate += baseUpdatePeriod;
		}
		if (now >= nextResilienceUpdate) {
			resilienceDecrease = resilienceUpdatePeriod;
			nextResilienceUpdate += resilienceUpdatePeriod;
		}
		
		for (it = groups.begin(); it != groups.end(); ++it) {
			it->second->updateControlValues(updateGeneral, resilienceDecrease);
			it->second->processNewViews();
			it->second->processStatusChangesAndUpdateStatistics();
		}
		
		for (it = groups.begin(); it != groups.end(); ++it) {
			it->second->propagateVisibleViews();
		}
		
		for (it = groups.begin(); it != groups.end(); ++it) {
			it->second->presentNewlySalientObjects();
		}
		
		for (it = groups.begin(); it != groups.end(); ++it) {
			it->second->cleanup();
		}
		
		core->resume();
	}

	void ObjectBase::retain() { ++refCount; }
	void ObjectBase::release() {
		if (--refCount == 0) {
			// refCount=0 implies that we don't have any markers or views.
			delete this;
		}
	}

	ObjectBase* Impl::insertObject(ObjectBase* object)
	{
		if (object->type == ObjectBase::GROUP) {
			GroupImpl* group = reinterpret_cast<GroupImpl*>(object);
			group->coreInstance = core->createInstance(group);
			return object;
		} else {
			ObjectImpl* obj = reinterpret_cast<ObjectImpl*>(object);
			for (vector<ObjectBase*>::iterator it = obj->references.begin(); it != obj->references.end(); ++it) {
				*it = insertObject(*it);
			}
			pair<ObjectStore::iterator, bool> ins = objects.insert(obj);
			if (ins.second) {
				return obj;
			} else {
				// NOTE: we may want to tell the System about the collision
				return *ins.first;
			}
		}
	}
	
	void newView(ViewImpl* view)
	{
		view->isExisting = false;
		view->isSalient = false;
		view->isActive = false;
		view->isVisible = false;
		view->object->retain();
		view->group->retain();
		if (view->originGroup != 0)
			view->originGroup->retain();
		view->group->newContent.push_back(view);
	}
	
	void Impl::processInsertions()
	{
		int64 now = mBrane::Time::Get();
		InsertionStore objs;
		insertionQueueMutex.acquire();
			objs.swap(insertionQueue);
		insertionQueueMutex.release();
		for (InsertionStore::const_iterator it = objs.begin(); it != objs.end(); ++it) {
			ObjectBase* base = reinterpret_cast<ObjectBase*>(it->object);
			base = insertObject(base);
			ViewImpl* view = new ViewImpl();
			view->object = base;
			view->group = it->destination;
			view->originGroup = 0;
			view->originNodeID = it->node_id;
			view->injectionTime = now;
			view->saliency.value = it->viewData[0].asFloat();
			view->resilience.value = it->viewData[1].asFloat();
			if (it->viewData.size() > 1)
				view->activationOrVisibility.value = it->viewData[2].asFloat();
			if (it->viewData.size() > 2)
				view->copyOnVisibility = it->viewData[3].asFloat();
			newView(view);
		}
	}
	
	ObjectImpl::~ObjectImpl()
	{
		for (vector<ObjectBase*>::iterator it = references.begin(); it != references.end(); ++it)
			(*it)->release();
	}
	
	Expression ObjectBase::copyMarkerSet(ReductionInstance& dest) const
	{
		if (dest.value.size() < dest.input.size())
			dest.value.resize(dest.input.size());
		Expression result(&dest, dest.value.size());
		dest.value.push_back(Atom::Set(markers.size()));
		for (MarkerStore::const_iterator it = markers.begin(); it != markers.end(); ++it) {
			dest.value.push_back(Atom::RPointer(dest.references.size()));
			dest.references.push_back(*it);
		}
		return result;
	}

	Expression ObjectBase::copyViewSet(ReductionInstance& dest) const
	{
		vector<int> viewIndices;
		ViewStore::const_iterator it;
		for (it = views.begin(); it != views.end(); ++it) {
			ViewImpl* view = it->second;
			Expression e = copyViewInternal(dest, view);
			viewIndices.push_back(e.getIndex());
		}
		
		dest.value.resize(dest.input.size());
		Expression result(&dest, dest.value.size());
		dest.value.push_back(Atom::Set(viewIndices.size()));
		for (vector<int>::iterator it = viewIndices.begin(); it != viewIndices.end(); ++it) {
			dest.value.push_back(Atom::IPointer(*it));
		}
		return result;
	}

	float transformControlValue(float value, float originThreshold, float destinationThreshold)
	{
		if (value < originThreshold) {
			return value * destinationThreshold / originThreshold;
		} else if (value > originThreshold) {
			return 1 - (1 - value) * (1 - destinationThreshold) / (1 - originThreshold);
		} else { // value == originThreshold
			return destinationThreshold;
		}
	}
	
	Expression ObjectBase::copyVisibleView(ReductionInstance& dest, const Group* group) const
	{
		ViewImpl transformedView;
		ViewImpl* view = 0;
		ViewStore::const_iterator it = views.find(group);
		if (it != views.end()) {
			view = it->second;
		} else {
			for (it = views.begin(); it != views.end(); ++it) {
				GroupImpl* otherGroup = it->second->group;
				if (otherGroup->cSaliency > otherGroup->cSaliencyThreshold) {
					ViewStore::const_iterator it2 = otherGroup->views.find(group);
					if (it2 != otherGroup->views.end()) {
						ViewImpl* groupView = it2->second;
						GroupImpl* groupImpl = groupView->group;
						// QUESTION: what about c-saliency of otherGroup?
						if (groupView->activationOrVisibility.value > groupImpl->visibilityThreshold) {
							float thisSaliency = transformControlValue(
								groupView->saliency.value,
								otherGroup->saliencyThreshold,
								groupImpl->saliencyThreshold
							);
							if (!view || thisSaliency >= view->saliency.value) {
								transformedView = *it->second;
								transformedView.saliency.value = thisSaliency;
								if (type == GROUP) {
									transformedView.activationOrVisibility.value = transformControlValue(
										groupView->activationOrVisibility.value,
										otherGroup->visibilityThreshold,
										groupImpl->visibilityThreshold
									);
								} else if (type == REACTIVE) {
									transformedView.activationOrVisibility.value = transformControlValue(
										groupView->activationOrVisibility.value,
										otherGroup->activationThreshold,
										groupImpl->activationThreshold
									);
								}
								view = &transformedView;
							}
						}
					}
				}
			}
		}
		return copyViewInternal(dest, view);
	}

	Expression ObjectBase::copyLocalView(ReductionInstance& dest, const Group* group) const
	{
		ViewStore::const_iterator it = views.find(group);
		ViewImpl* view = (it == views.end()) ? 0 : it->second;
		return copyViewInternal(dest, view);
	}
	
	Expression ObjectBase::copyViewInternal(ReductionInstance& dest, const ViewImpl* view) const
	{
		if (dest.value.size() < dest.input.size())
			dest.value.resize(dest.input.size());
		Expression ijt(&dest, dest.value.size());
		dest.value.push_back(Atom::Timestamp());
		dest.value.push_back(Atom(view->injectionTime >> 32));
		dest.value.push_back(Atom(view->injectionTime));
		Expression res(&dest, dest.value.size());
		int64 res64 = view->resilience.value;
		dest.value.push_back(Atom::Timestamp());
		dest.value.push_back(Atom(res64 >> 32));
		dest.value.push_back(Atom(res64));
		Expression result(&dest, dest.value.size());
		int numAtoms = (type == OBJECT) ? 5 : (type == REACTIVE) ? 6 : 7;
		dest.value.push_back(Atom::Set(numAtoms));
		dest.value.push_back(ijt.iptr());
		dest.value.push_back(Atom::Float(view->saliency.value));
		dest.value.push_back(res.iptr());
		dest.value.push_back(Atom::RPointer(dest.references.size()));
		dest.references.push_back(view->group);
		if (view->originGroup != 0) {
			dest.value.push_back((Atom::RPointer(dest.references.size())));
			dest.references.push_back(view->originGroup);
		} else {
			dest.value.push_back(Atom::Node(view->originNodeID));
		}
		if (type != OBJECT)
			dest.value.push_back(Atom::Float(view->activationOrVisibility.value));
		if (type == GROUP)
			dest.value.push_back(Atom::Float(view->copyOnVisibility));
		return result;
	}
	
	void ObjectBase::prepareForCopy(ReductionInstance& dest) const
	{
		Object* o = const_cast<Object*>(static_cast<const Object*>(this));
		if (dest.references.size() == 0)
			dest.references.push_back(o);
		else if (dest.input.size() < dest.value.size())
			dest.input.resize(dest.value.size());
		ReductionInstance::CopiedObject co;
		co.object = o;
		co.position = dest.input.size();
		dest.copies.push_back(co);
	}
	
	GroupImpl::GroupImpl(Core* core, Mem* mem_)
	{
		type = GROUP;
		coreInstance = core->createInstance(this);
		mem = mem_;
		numGeneralUpdatesSkipped = 0;
		updateCounter = 0;
	}

	GroupImpl::~GroupImpl()
	{
		delete coreInstance;
	}
	
	Expression GroupImpl::copy(ReductionInstance& dest) const
	{
		prepareForCopy(dest);
		
		dest.input.push_back(opcodeRegister["grp"]);
		for (const float* f = values; f != &lastValueSentinel; ++f) {
			dest.input.push_back(Atom::Float(*f));
		}
		if (notificationGroup == 0) {
			dest.input.push_back(Atom::Nil());
		} else {
			dest.input.push_back(Atom::RPointer(dest.references.size()));
			dest.references.push_back(notificationGroup);
		}
		dest.input.push_back(Atom::View());
		dest.input.push_back(Atom::Mks());
		dest.input.push_back(Atom::Vws());
		dest.input.push_back(Atom::Float(propagationSaliencyThreshold));
	}

	Object* GroupImpl::getReference(int index) const { return 0; }

	const Group* GroupImpl::asGroup() const { return this; }

	void GroupImpl::receive(ReductionInstance* instance)
	{
		reductions.push_back(instance);
	}
	
	void GroupImpl::processInjectionOrEjection(ReductionInstance* ri, Expression command)
	{
		Expression args(command.child(3));
		Expression objectExpr(args.child(1));
		Expression viewExpr(args.child(2));
		Object* object = ri->objectForExpression(objectExpr);
		ViewImpl* view = new ViewImpl();
		view->object = reinterpret_cast<ObjectImpl*>(object);
		view->group = reinterpret_cast<GroupImpl*>(ri->references[viewExpr.child(1).head().asIndex()]);
		view->originGroup = this;
		view->originNodeID = 0;
		view->injectionTime = viewExpr.child(3).decodeTimestamp();
		view->saliency.value = viewExpr.child(4).head().asFloat();
		view->resilience.value = viewExpr.child(5).decodeTimestamp();
		if (viewExpr.head().getAtomCount() >= 6)
			view->activationOrVisibility.value = viewExpr.child(6).head().asFloat();
		if (viewExpr.head().getAtomCount() >= 7)
			view->copyOnVisibility = viewExpr.child(7).head().asFloat();
		// TODO: check for _inj or _eje
		if (1) {
			newView(view);
		} else {
			// TODO: _eje
		}
	}
	
	void GroupImpl::processModOrSet(ReductionInstance* ri, Expression command)
	{
		// TODO
	}
	
	void GroupImpl::processCommands()
	{
		for (vector<ReductionInstance*>::iterator it = reductions.begin(); it != reductions.end(); ++it) {
			Expression ip(*it, 0, true);
			Expression commands(ip.child(1).child(3));
			for (int i = 1; i <= commands.head().getAtomCount(); ++i) {
				Expression command(commands.child(i));
				// TODO: check for which command is to be done
				processInjectionOrEjection(*it, command);
				processModOrSet(*it, command);
			}
		}
	}
	
	void GroupImpl::addNotificationMarker(ObjectImpl* obj, ViewImpl* view)
	{
		GroupImpl* group = view->group;
		ViewImpl* markerView = new ViewImpl;
		markerView->object = obj;
		if (group->notificationGroup != 0)
			markerView->group = group->notificationGroup;
		else
			markerView->group = group;
		markerView->originGroup = group;
		markerView->originNodeID = 0;
		markerView->injectionTime = mBrane::Time::Get();
		markerView->saliency.value = 1;
		newView(markerView);
	}
	
	void GroupImpl::processNotifications(
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
	) {
		if (updateCounter % changePeriod == 0) {
			float change = value.value - value.valueAtLastChangePeriod;
			if (fabs(change) >= changeThreshold) {
				ObjectImpl* o = new ObjectImpl;
				o->atoms.push_back(Atom::Marker(changeMarkerOpcode, 2));
				o->atoms.push_back(Atom::RPointer(0));
				o->atoms.push_back(Atom::Float(change));
				o->references.push_back(view->object);
				addNotificationMarker(o, view);
			}
			value.valueAtLastChangePeriod = value.value;
		}
		if (updateCounter % valuePeriod == 0) {
			if (value.value < lowValueThreshold) {
				if (!value.lowValueNotified) {
					value.lowValueNotified = true;
					ObjectImpl* o = new ObjectImpl;
					o->atoms.push_back(Atom::Marker(lowValueMarkerOpcode, 1));
					o->atoms.push_back(Atom::RPointer(0));
					o->references.push_back(view->object);
					addNotificationMarker(o, view);
				}
			} else {
				value.lowValueNotified = false;
			}
			if (value.value > highValueThreshold) {
				if (!value.highValueNotified) {
					value.highValueNotified = true;
					ObjectImpl* o = new ObjectImpl;
					o->atoms.push_back(Atom::Marker(highValueMarkerOpcode, 1));
					o->atoms.push_back(Atom::RPointer(0));
					o->references.push_back(view->object);
					addNotificationMarker(o, view);
				}
			} else {
				value.highValueNotified = false;
			}
		}
	}
	
	void GroupImpl::updateControlValues(bool updateGeneral, int64 resilienceDecrease)
	{
		if (updateGeneral && ++numGeneralUpdatesSkipped < updatePeriod)
			updateGeneral = false;
		++updateCounter;
		controlValuesChanged = updateGeneral;
		if (!updateGeneral && resilienceDecrease == 0)
			return;
		
		for (ContentStore::iterator it = content.begin(); it != content.end(); ) {
			ViewImpl* view = it->second;
			
			if (resilienceDecrease != 0) {
				if (view->mediations[1].requestCount != 0) {
					view->resilience.value = view->mediations[1].getAndReset();
				}
				if (view->resilience.value == -1) {
					// do nothing
				} else if (view->resilience.value == 0) {
					content.erase(it++);
					// TODO: check for a non-salient view which was shadowing a salient view
					continue;
				} else if (view->resilience.value <= resilienceDecrease) {
					view->resilience.value = 0;
				} else {
					view->resilience.value -= resilienceDecrease;
				}
			}
			if (updateGeneral) {
				if (view->saliencyPeriodsOfDecayRemaining > 0 && updateCounter % int(saliencyDecayPeriod) == 0) {
					--view->saliencyPeriodsOfDecayRemaining;
					view->mediations[0].requestCount;
					view->mediations[0].sumTargetValue += view->saliency.value - view->saliencyDecayPerPeriod;
				}
				if (view->mediations[0].requestCount != 0) {
					view->saliency.value = view->mediations[0].getAndReset();
				}
				if (view->mediations[2].requestCount != 0) {
					view->activationOrVisibility.value = view->mediations[2].getAndReset();
				}
				processNotifications(
					view->saliency,
					updateCounter,
					saliencyChangeNotificationThreshold,
					saliencyChangeNotificationPeriod,
					saliencyLowValueThreshold,
					saliencyHighValueThreshold,
					saliencyValueNotificationPeriod,
					MARKER_SALIENCY_CHANGE,
					MARKER_SALIENCY_LOW_VALUE,
					MARKER_SALIENCY_HIGH_VALUE,
					view
				);
				processNotifications(
					view->resilience,
					updateCounter,
					1e100,
					1,
					resilienceLowValueThreshold,
					resilienceHighValueThreshold,
					resilienceValueNotificationPeriod,
					-1,
					MARKER_RESILIENCE_LOW_VALUE,
					MARKER_RESILIENCE_HIGH_VALUE,
					view
				);
				if (view->object->type == REACTIVE) {
					processNotifications(
						view->activationOrVisibility,
						updateCounter,
						activationChangeNotificationThreshold,
						activationChangeNotificationPeriod,
						activationLowValueThreshold,
						activationHighValueThreshold,
						activationValueNotificationPeriod,
						MARKER_ACTIVATION_CHANGE,
						MARKER_ACTIVATION_LOW_VALUE,
						MARKER_ACTIVATION_HIGH_VALUE,
						view
					);
				}
			}
			++it;
		}
	}
	
	void GroupImpl::processNewViews()
	{
		for (vector<ViewImpl*>::iterator itNew = newContent.begin(); itNew != newContent.end(); ++itNew) {
			ViewImpl* view = *itNew;
			ContentStore::iterator itExisting = content.find(view->object);
			if (itExisting != content.end()) {
				ViewImpl* existingView = itExisting->second;
				if (view->saliency.value <= existingView->saliency.value)
					continue;
				existingView->object->release();
				existingView->group->release();
				if (existingView->originGroup != 0)
					existingView->originGroup->release();
				view->isExisting = existingView->isExisting;
				view->isSalient = existingView->isSalient;
				view->isActive = existingView->isActive;
				view->isVisible = existingView->isVisible;
				*existingView = *view;
				delete view;
				view = existingView;
			} else {
				content[view->object] = view;
				view->object->views[this] = view;
				if (view->object->type == OBJECT) {
					ObjectImpl* obj = reinterpret_cast<ObjectImpl*>(view->object);
					if (obj->atoms[0].getDescriptor() == Atom::MARKER) {
						for (int i = 0; i < obj->references.size(); ++i) {
							obj->references[i]->markers.insert(obj);
						}
					}
				}
			}
			insertedContent.push_back(view);
		}
	}

	void GroupImpl::makeGroupVisible(ViewImpl* view)
	{
		GroupImpl* group = view->group;
		if (group->cSaliency <= group->cSaliencyThreshold)
			return;
		
		for (ContentStore::iterator it = group->content.begin(); it != group->content.end(); ++it) {
			ViewImpl* v = it->second;
			if (v->isSalient) {
				combinedNewlySalientContent.insert(v);
			}
		}
	}
	
	void GroupImpl::processStatusChangeAndUpdateStatistics(ViewImpl* view)
	{
		if (cSaliency > cSaliencyThreshold
		 && view->saliency.value > saliencyThreshold) {
			if (!view->isSalient)
				localNewlySalientContent.push_back(view);
		} else {
			view->isSalient = false;
		}
		if (view->object->type == REACTIVE) {
			if (cActivation > cActivationThreshold
			 && view->activationOrVisibility.value > activationThreshold) {
				if (!view->isActive) {
					coreInstance->activate(view->object);
					view->isActive = true;
				}
			} else {
				if (view->isActive) {
					coreInstance->deactivate(view->object);
					view->isActive = false;
				}
			}
			
		} else if (view->object->type == GROUP) {
			if (cSaliency > cSaliencyThreshold
			 && view->activationOrVisibility.value > visibilityThreshold) {
				if (!view->isVisible) {
					makeGroupVisible(view);
					view->isVisible = true;
				}
			} else {
				if (view->isVisible) {
					view->isVisible = false;
				}
			}
		}
		if (view->saliency.value < saliencyLowValue)
			saliencyLowValue = view->saliency.value;
		if (view->saliency.value > saliencyHighValue)
			saliencyHighValue = view->saliency.value;
		sumSaliency += view->saliency.value;
		++numObjects;
		if (view->object->type == REACTIVE) {
			if (view->activationOrVisibility.value < activationLowValue)
				activationLowValue = view->activationOrVisibility.value;
			if (view->activationOrVisibility.value > activationHighValue)
				activationHighValue = view->activationOrVisibility.value;
			sumActivation += view->activationOrVisibility.value;
			++numReactiveObjects;
		}
	}
	
	void GroupImpl::processStatusChangesAndUpdateStatistics()
	{
		if (!controlValuesChanged) {
			for (vector<ViewImpl*>::iterator it = insertedContent.begin(); it != insertedContent.end(); ++it) {
				ViewImpl* view = *it;
				processStatusChangeAndUpdateStatistics(*it);
			}
		} else {
			numObjects = numReactiveObjects = 0;
			sumSaliency = sumActivation = 0.0;
			saliencyLowValue = activationLowValue = 1;
			saliencyHighValue = activationHighValue = 0;
			for (ContentStore::iterator it = content.begin(); it != content.end(); ++it) {
				processStatusChangeAndUpdateStatistics(it->second);
			}
		}
		if (numObjects > 0) {
			saliencyAverageValue = sumSaliency / numObjects;
		} else {
			saliencyAverageValue = 0;
		}
		if (numReactiveObjects > 0) {
			activationAverageValue = sumActivation / numObjects;
		} else {
			activationAverageValue = 0;
		}
	}

	void GroupImpl::propagateVisibleViews()
	{
		for(vector<ViewImpl*>::iterator it = localNewlySalientContent.begin(); it != localNewlySalientContent.end(); ++it) {
			combinedNewlySalientContent.insert(*it);
		}
		for (ViewStore::iterator itView = views.begin(); itView != views.end(); ++itView) {
			GroupImpl* group = itView->second->group;
			if (itView->second->isVisible
			 && group->cSaliency > group->cSaliencyThreshold
			 && group->cActivation > group->cActivationThreshold) {
				for(vector<ViewImpl*>::iterator it = localNewlySalientContent.begin(); it != localNewlySalientContent.end(); ++it) {
					group->combinedNewlySalientContent.insert(*it);
				}
			}
		}
	}

	void GroupImpl::presentNewlySalientObjects()
	{
		for (SalientStore::iterator itS = combinedNewlySalientContent.begin(); itS != combinedNewlySalientContent.end(); ++itS) {
			ViewImpl* view = *itS;
			bool foundSalientVisibleView = false;
			if (view->group != this || !view->isExisting) {
				ObjectBase* base = view->object;
				for (ViewStore::iterator itV = base->views.begin(); itV != base->views.end(); ++itV) {
					ViewImpl* otherView = itV->second;
					if (otherView->isSalient) {
						ViewStore::iterator itGroupView = views.find(otherView->group);
						if (itGroupView != views.end() && itGroupView->second->isVisible) {
							foundSalientVisibleView = true;
							break;
						}
					}
				}
			}
			if (!foundSalientVisibleView) {
				coreInstance->salientObject(view->object);
			}
		}
	}

	void GroupImpl::cleanup()
	{
		for (vector<ViewImpl*>::iterator it = insertedContent.begin(); it != insertedContent.end(); ++it)
			(*it)->isExisting = true;
		for (vector<ViewImpl*>::iterator it = localNewlySalientContent.begin(); it != localNewlySalientContent.end(); ++it)
			(*it)->isSalient = true;
		reductions.clear();
		newContent.clear();
		insertedContent.clear();
		localNewlySalientContent.clear();
		combinedNewlySalientContent.clear();
	}

	Expression ObjectImpl::copy(ReductionInstance& dest) const
	{
		prepareForCopy(dest);
		for (vector<Atom>::const_iterator it = atoms.begin(); it != atoms.end(); ++it)
			dest.input.push_back(*it);
	}

	Object* ObjectImpl::getReference(int index) const
	{
		if (index >= 0 && index < references.size())
			return references[index];
		else
			return 0;
	}
	
	size_t ObjectImpl::hash::operator()(const ObjectImpl* obj) const
	{
		if (obj->hash_value == 0) {
			size_t hv = 0;
			for (int i = 0; i < obj->references.size(); ++i) {
				const ObjectBase* base = reinterpret_cast<const ObjectBase*>(obj->references[i]);
				if (base->type == ObjectBase::GROUP)
					hv = hv * 5 + reinterpret_cast<size_t>(base);
				else
					hv = hv * 5 + hash()(reinterpret_cast<const ObjectImpl*>(base));
			}
			for (int i = 0; i < obj->atoms.size(); ++i) {
				hv = hv * 5 + obj->atoms[i].atom;
			}
			const_cast<ObjectImpl*>(obj)->hash_value = hv;
		}
		return obj->hash_value;
	}
	
	bool ObjectImpl::equal_object::operator()(const ObjectImpl* a, const ObjectImpl* b) const
	{
		return a->references == b->references && a->atoms == b->atoms;
	}
	
	float MediatedValueAccumulator::getAndReset()
	{
		float value = sumTargetValue / requestCount;
		sumTargetValue = 0;
		requestCount = 0;
		return value;
	}
}

UNORDERED_MAP<std::string, r_code::Atom> opcodeRegister;

Mem* Mem::create(
	UNORDERED_MAP<std::string, r_code::Atom> classes,
	std::vector<r_code::Object*> objects,
	ObjectReceiver *r
) {
	opcodeRegister = classes;
	return new MemImpl::Impl(r, objects);
}

Object* Object::create(std::vector<Atom> atoms, std::vector<Object*> references)
{
	MemImpl::ObjectImpl* obj = new MemImpl::ObjectImpl();
	for (int i = 0; i < references.size(); ++i)
		obj->references.push_back( reinterpret_cast<MemImpl::ObjectBase*>(references[i]));
	obj->atoms = atoms;
	int opcode = atoms[0].asOpcode();
	if (opcode == OPCODE_IPGM)
		obj->type = MemImpl::ObjectBase::REACTIVE;
	return obj;

} 
 }
