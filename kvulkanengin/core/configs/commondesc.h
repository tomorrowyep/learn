#ifndef __COMMOND_DESC_H__
#define __COMMOND_DESC_H__

enum class QueueType
{
	Graphics,
	Present,
	//Compute,
	//Transfer
};

struct QueueFamilyIndexs
{
	int _graphicsFamily = -1;
	int _presentFamily = -1;

	bool IsComplete() const
	{
		return _graphicsFamily >= 0 && _presentFamily >= 0;
	}
};

#endif // !__COMMOND_DESC_H__
