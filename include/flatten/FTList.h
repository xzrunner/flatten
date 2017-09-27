#ifndef _FLATTEN_LIST_H_
#define _FLATTEN_LIST_H_

#include <cooking/DisplayList.h>

#include <vector>
#include <memory>

#include <stdint.h>

namespace s2 { class Sprite; class Actor; class RenderParams; }

namespace ft
{

class FTNode;

class FTList : public std::enable_shared_from_this<FTList>
{
public:
	FTList(s2::Actor* root);
	FTList(const FTList&) = delete;
	FTList(FTList&&) = delete;
	~FTList();

	bool Update(int pos, bool force, const std::shared_ptr<cooking::DisplayList>& dlist);
	void DrawForward(int pos, const s2::RenderParams& rp);
	void DrawDeferred(int pos, const s2::RenderParams& rp,
		const std::shared_ptr<cooking::DisplayList>& dlist);

	void SetFrame(int pos, bool force, int frame,
		const std::shared_ptr<cooking::DisplayList>& dlist);

	void SetDirty() { m_dirty = true; }

private:
	void Build(const std::shared_ptr<cooking::DisplayList>& dlist);

	void InitNeedUpdateFlag();

	bool CheckFirst(int pos, const std::shared_ptr<cooking::DisplayList>& dlist);

	int  CalcDListAllCount(const cooking::DisplayList& dlist, int pos) const;

	void SetDrawlistDirty(const FTNode* node);

private:
	static const int MAX_LAYER = 16;

private:
	s2::Actor* m_root;

	FTNode* m_nodes;
	int m_nodes_sz, m_nodes_cap;

	int m_max_layer;

	bool m_dirty;

	friend class BuildListVisitor;

}; // FTList

}

#endif // _FLATTEN_FLATTEN_H_
