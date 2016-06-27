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


		class Updater
		{
			public:
				bool empty() const { return _observers.empty(); }
				void attach(playerDataPtr d);
				void detach(playerDataPtr d);
				virtual void tick() = 0;

			protected:
				std::set<int> _observers;
		};
	
		class Timer
		{
			public:
				typedef boost::function<void()> TickFunc;
			private:
				struct Item
				{
					Item(unsigned tick_time, const TickFunc& func)
						: _tick_time(tick_time), _tick_func(func){}
					
					bool operator<(const Item& rhs) const
					{
						return _tick_time > rhs._tick_time;
					}
	
					unsigned _tick_time;
					TickFunc _tick_func;
				};
			public:
				void add(unsigned tick_time, const TickFunc& func)
				{
					_queue.push(Item(tick_time, func));
				}

				void check(unsigned cur_time)
				{
					while(!_queue.empty() &&
						cur_time >= _queue.top()._tick_time)
					{
						_queue.top()._tick_func();
						_queue.pop();
					}
				}

			private:
				priority_queue<Item> _queue;
		};
	}
}
