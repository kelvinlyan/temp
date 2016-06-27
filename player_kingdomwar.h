#pragma once

#include "kingdomwar_def.h"
#include "auto_base.h"
#include "mongoDB.h"

namespace gg
{
	class playerMan;

	namespace KingdomWar
	{
		enum{
			PosEmpty = -1,
			PosCity,
			PosPath,
			PosSiege,
		};
		
		struct Position
		{
			Position(): _type(PosEmpty){}

			mongo::BSONObj toBSON() const;
			void load(const mongo::BSONElement& obj);

			int _type;
			int _id;
			unsigned _time;
			int _from_city_id;
		};

		SHAREPTR(Position, PositionPtr);
		
		SHAREPTR(playerMan, playerManPtr);
		STDVECTOR(playerManPtr, ManList);

		class Formation
			: public _auto_player
		{
			public:

				Formation(int army_id, playerData* const own);

				virtual void _auto_update();
				virtual bool _auto_save(); 
				void update();

				mongo::BSONObj toBSON() const;
				mongo::BSONArray manListBSON() const;
				void load(const mongo::BSONElement& obj);

				int armyId() const { return _army_id; }
				void getInfo(qValue& q) const;
				int setFormation(int fm_id, const int fm[9]);
				const ManList& manList() const { return _man_list; }

				void recalFM();

			private:
				const int _army_id;
				int _fm_id;
				int _power;
				ManList _man_list;
		};

		SHAREPTR(Formation, FormationPtr);

		struct Army
		{
			Army(int id, playerData* const own);

			mongo::BSONObj toBSON() const;
			void load(const mongo::BSONElement& obj);

			int _id;
			int _hp;
			PositionPtr _pos;
			FormationPtr _fm;
		};

		SHAREPTR(Army, ArmyPtr);
		STDVECTOR(ArmyPtr, Armys);
	}

	class playerKingdomWar
		: public _auto_player
	{
		public:
			playerKingdomWar(playerData* const own);

			bool inited() const;
			void initData();
			void loadDB();
			virtual bool _auto_save();
			virtual void _auto_update();

			int manHp(int man_id) const;
			void setManHp(int man_id, int hp);
			const KingdomWar::ManList& getFM(int army_id) const;
			
			int moveable(int army_id, int to_city_id);
			void setPosition(int army_id, int type, int id, unsigned time, int from_city_id = -1);

			bool isDead(int army_id);
			KingdomWar::PositionPtr getPosition(int army_id);
			int getBV(int army_id);
			bool manUsed(int army_id, int man_id);

		private:
			STDMAP(int, int, Man2HpMap);
			Man2HpMap _man_hp;
			KingdomWar::Armys _armys;
	};
}
