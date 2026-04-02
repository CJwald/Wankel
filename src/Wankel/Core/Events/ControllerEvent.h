#pragma once

#include "Wankel/Core/Events/Event.h"

namespace Wankel {

	class ControllerEvent : public Event {
	public:
		int GetControllerID() const { return m_ControllerID; }

		EVENT_CLASS_CATEGORY(EventCategoryController)
	protected:
		ControllerEvent(int controllerID)
			: m_ControllerID(controllerID) {}

		int m_ControllerID;
	};

}