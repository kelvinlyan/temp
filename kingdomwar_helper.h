#pragma once

#include "commom.h"

namespace gg
{
	class playerData;
	SHAREPTR(playerData, playerDataPtr);

	namespace KingdomWar
	{
		inline int divide32(int x, int y)
		{
			return (x - 1) / y + 1;
		}

		typedef long long I64;
		inline I64 divide64(I64 x, I64 y)
		{
			return (x - 1) / y + 1;
		}

		class Graph
		{
			public:
				enum{
					Max = 999999
				};
				Graph(unsigned n)
				{
					_n = n;
					_ptr = new int[n * n];
					for (unsigned i = 0; i < n; ++i)
						for (unsigned j = 0; j < n; ++j)
							_ptr[i * n + j] = Max;
				}
				~Graph()
				{
					delete [] _ptr;
				}

				void set(int i, int j, int v) { _ptr[i * _n + j] = v; }
				unsigned num() const { return _n; }
				int line(int i, int j) const { return _ptr[i * _n + j]; }

			private:
				int* _ptr;
				unsigned _n;
		};
		
		STDVECTOR(int, Line);
		STDVECTOR(Line, Lines);
		void shortestPath_DIJ(const Graph& graph, Lines& lines, int start_id);
		
		class LineInfo
		{
			public:
				LineInfo(): _cost_time(-1){}

				void set(const Line& line);
				bool empty() const { return _cost_time == -1; }
				int costTime() const { return _cost_time; }
				const Line& get() const { return _line; }

			private:
				int _cost_time;
				Line _line;
		};

		STDVECTOR(LineInfo, LineInfos);
		STDVECTOR(LineInfos, LineInfosList);

		class ShortestPath
		{
			public:
				void load(const vector<Lines>& lines);
				const LineInfo& getLine(int from_id, int to_id) const;

			private:
				LineInfosList _line_info;
				static LineInfo _empty;
		};
		
		class Observer
		{
			public:
				typedef std::set<int> IdList;

				void attach(int id) { _id_list.insert(id); }
				void detach(int id) { _id_list.erase(id); }
				bool empty() const { return _id_list.empty(); }
				IdList& getObserver() { return _id_list; }

			private:
				IdList _id_list;
		};
	}
}
