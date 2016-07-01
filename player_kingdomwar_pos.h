#pragma once

#include "auto_base.h"
#include "mongoDB.h"
#include "kingdomwar_helper.h"

namespace gg
{
	namespace KingdomWar
	{
		struct Position
		{
			Position();

			int _type;
			int _id;
			unsigned _time;
			int _from_city_id;
			STDLIST(int, IdList);
			IdList _line;
		};
	}

	class playerKingdomWarSGPos
		: public _auto_player
	{
		public:
			playerKingdomWarSGPos(int army_id, playerData* const own);

			void load(const mongo::BSONObj& obj);
			void getInfo(qValue& q);

			const KingdomWar::Position& position() const { return _pos; }
			void setPosition(int type, int id, unsigned time);
			void setLine(const KingdomWar::Line& line);
			void updateHp() { _sign_update(); }
			int move(unsigned time, int to_city_id, bool start);
			int retreat();

		private:
			virtual bool _auto_save();
			virtual void _auto_update();

		private:
			const int _army_id;
			KingdomWar::Position _pos;
	};

	SHAREPTR(playerKingdomWarSGPos, playerKingdomWarSGPosPtr);
	STDVECTOR(playerKingdomWarSGPosPtr, PlayerKingdomWarPosList);

	class playerKingdomWarPos
		: public _auto_player
	{
		public:
			playerKingdomWarPos(playerData* const own);

			void loadDB();
			void update();

			const KingdomWar::Position& position(int army_id) const 
			{
				return _pos_list[army_id]->position(); 
			}

			void setPosition(int army_id, int type, int id, unsigned time);
			void setLine(int army_id, const KingdomWar::Line& l);
			void updateHp(int army_id) { _pos_list[army_id]->updateHp(); }
			int move(unsigned time, int army_id, int to_city_id, bool start);
			int retreat(int army_id);

		private:
			virtual bool _auto_save();
			virtual void _auto_update();

		private:
			PlayerKingdomWarPosList _pos_list;
	};
}
