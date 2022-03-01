#include "Sash.h"
#include "Node.h"

bool Sash::Contains(const wxPoint& pt)
{
	return 
		pt.x >= this->pos.x &&
		pt.y >= this->pos.y &&
		pt.x <= this->pos.x + this->size.x &&
		pt.y <= this->pos.y + this->size.y;
}

void Sash::SlideHoriz(int amt)
{
	this->left->cacheSize.x += amt;
	this->right->cachePos.x += amt;
	this->right->cacheSize.x -= amt;

	this->pos.x += amt;
}

void Sash::SlideVert(int amt)
{
	this->top->cacheSize.y += amt;
	this->bot->cachePos.y += amt;
	this->bot->cacheSize.y -= amt;

	this->pos.y += amt;
}