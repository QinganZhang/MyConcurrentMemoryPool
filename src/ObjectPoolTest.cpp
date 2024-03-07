#include "ObjectPool.h"
// 比较实现的定长内存池和malloc/free的性能，进行测试：
struct TreeNode
{
	int _val;
	int a[500];
	TreeNode* _left;
	TreeNode* _right;
	TreeNode() :_val(0), _left(nullptr), _right(nullptr) {}
};

void TestObjectPool(){
	// 申请释放的轮次
	const size_t Rounds = 3;
	// 每轮申请释放多少次
	const size_t N = 200000;
	std::vector<TreeNode*> v1;
	v1.reserve(N);

	//malloc和free
	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}
	size_t end1 = clock();

	//定长内存池
	ObjectPool<TreeNode> TNPool; // TreeNode的Generator
	std::vector<TreeNode*> v2;
	v2.reserve(N);
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}
	size_t end2 = clock();

	cout << "new/delete cost time: " << (end1 - begin1) / (double)CLOCKS_PER_SEC << "ms" << endl;
	cout << "ObjectPool cost time: " << (end2 - begin2) / (double)CLOCKS_PER_SEC << "ms" << endl;
}

// int main(){
//     TestObjectPool();
//     return 0;
// }