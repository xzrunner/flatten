#ifndef _FLATTEN_BUILD_LIST_VISITOR_H_
#define _FLATTEN_BUILD_LIST_VISITOR_H_

#include <sprite2/s2_config.h>
#include <sprite2/SpriteVisitor.h>

#include <vector>
#include <memory>

namespace cooking { class DisplayList; }

namespace ft
{

class FTList;

class BuildListVisitor : public s2::SpriteVisitor
{
public:
	BuildListVisitor(const std::shared_ptr<FTList>& flatten, 
		const std::shared_ptr<cooking::DisplayList>& dlist);

	virtual s2::VisitResult Visit(const s2::SprConstPtr& spr, const s2::SprVisitorParams& params);

	virtual s2::VisitResult VisitChildrenBegin(const s2::SprConstPtr& spr, const s2::SprVisitorParams& params);
	virtual s2::VisitResult VisitChildrenEnd(const s2::SprConstPtr& spr, const s2::SprVisitorParams& params);

private:
	const std::shared_ptr<FTList>& m_flatten;
	const std::shared_ptr<cooking::DisplayList>& m_dlist;

	std::vector<int> m_curr_path;

}; // BuildFlattenVisitor

}

#endif // _FLATTEN_BUILD_LIST_VISITOR_H_