#pragma once

#include "auto_base.h"
#include "mongoDB.h"

namespace gg
{
	class playerKingdomWarBox
		: public _auto_player
	{
		public:
			playerKingdomWarBox(playerData* const own);
			void load(const mongo::BSONElement& obj);
			mongo::BSONArray toBSON() const;

			virtual bool _auto_save(){ return true; }
			virtual void _auto_update();
			void update();

			int getReward(int id);

		private:
			std::vector<int> _box_state;
	};

	class playerKingdomWarShop
		: public _auto_player
	{
		public:
			playerKingdomWarShop(playerData* const own);

			void loadDB();
			virtual bool _auto_save();
			virtual void _auto_update();

			void update();
			void updateBox();
			int buy(int pos, int id);
			int flush();
			int getBoxReward(int id);
			void dailyTick();

		private:
			void checkShopRecord();

			struct Record
			{
				Record(int id, int buy_times)
					: _id(id), _buy_times(buy_times){}

				int _id;	
				int _buy_times;
			};

			std::vector<Record> _records;
			unsigned _flush_times;
			int _used_exploit;
			SHAREPTR(playerKingdomWarBox, BoxPtr);
			BoxPtr _box_state;
	};
}
