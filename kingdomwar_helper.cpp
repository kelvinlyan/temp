#include "kingdomwar_helper.h"
#include "playerData.h"
#include "kingdomwar_system.h"

namespace gg
{
	namespace KingdomWar
	{
		void shortestPath_DIJ(const Graph& graph, Lines& lines, int start_id)
		{
			const unsigned n = graph.num();
			lines.clear();
			lines.insert(lines.end(), n, Line());
			vector<int> states(n, 0);
			vector<int> distances(n, Graph::Max);

			int current = start_id;
			states[current] = true;

			int minD = Graph::Max;
			int minP = current;

			for (unsigned i = 0; i < n; ++i)
			{
				if (states[i])
					continue;
				distances[i] = graph.line(current, i);
				if (distances[i] != Graph::Max)
				{
					lines[i].push_back(current);
					lines[i].push_back(i);
				}
				if (distances[i] < minD)
				{
					minD = distances[i];
					minP = i;
				}
			}

			for (unsigned i = 2; i < n; ++i)
			{
				current = minP;
				states[current] = true;
				minD = Graph::Max;
				minP = current;
				for (unsigned j = 0; j < n; ++j)
				{
					if (states[j])
						continue;
					int temp = graph.line(current, j) + distances[current];
					if (temp < distances[j])
					{
						distances[j] = temp;
						lines[j] = lines[current];
						lines[j].push_back(j);
					}
					if (distances[j] < minD)
					{
						minD = distances[j];
						minP = j;
					}
				}
			}

	/*		for (unsigned i = 0; i < lines.size(); ++i)
			{
				Line& l = lines[i];
				cout << "From " << start_id << " to " << i << " : ";
				for (unsigned j = 0; j < l.size(); ++j)
					cout << l[j] << " ";
				cout << endl;
			}
	*/
		}
		
		void LineInfo::set(const Line& line)
		{
			_line = line;
			if (!line.empty())
			{
				int from_id = line.front();
				for (unsigned i = 1; i < line.size(); ++i)
				{
					if (_cost_time == -1)
						_cost_time = kingdomwar_sys.getCostTime(from_id, line[i]);
					else
						_cost_time += kingdomwar_sys.getCostTime(from_id, line[i]);
					from_id = line[i];
				}
			}
		}

		LineInfo ShortestPath::_empty;

		void ShortestPath::load(const std::vector<Lines>& lines)
		{
			for (vector<Lines>::const_iterator iter = lines.begin(); iter != lines.end(); ++iter)
			{
				const Lines& p1 = *iter;
				LineInfos infos;
				for (Lines::const_iterator it1 = p1.begin(); it1 != p1.end(); ++it1)
				{
					LineInfo temp;
					temp.set(*it1);
					infos.push_back(temp);
				}
				_line_info.push_back(infos);
			}
		}

		const LineInfo& ShortestPath::getLine(int from_id, int to_id) const
		{
			if (from_id >= (int)_line_info.size() || from_id < 0)
				return _empty;
			if (to_id >= (int)_line_info.size() || to_id < 0)
				return _empty;
			return _line_info[from_id][to_id];
		}

	}
}
