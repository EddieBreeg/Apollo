#include "Element.hpp"

namespace apollo::ui {
	Element::Element(Clay_ElementId id, const Clay_ElementDeclaration& config)
	{
		Clay__OpenElementWithId(id);
		Clay__ConfigureOpenElementPtr(&config);
	}

	Element::~Element()
	{
		Clay__CloseElement();
	}
} // namespace apollo::ui