#pragma once

#include "dbDriver.h"
#include "commom.h"

#define kingdomwar_shop (*gg::kingdomwar_shop_system::_Instance)

namespace gg
{
	namespace KingdomWar
	{
		struct ShopData
		{	
			ShopData(const Json::Value& info);

			int _id;
			int	_consume_con;
			int _buy_times;
			int	_weight;
			Kingdom::NATION _kingdom_id;
			ACTION::BoxList _box;
		};

		SHAREPTR(ShopData, ShopDataPtr);
		STDMAP(int, ShopDataPtr, ShopDataMap);
		STDVECTOR(ShopDataPtr, ShopDataList);
		
		struct BoxData
		{	
			BoxData(const Json::Value& info);

			int _target;
			ACTION::BoxList _box;
		};

		SHAREPTR(BoxData, BoxDataPtr);
		STDVECTOR(BoxDataPtr, BoxDataMap);
	}

	class kingdomwar_shop_system
	{
		public:
			static kingdomwar_shop_system* const _Instance;

			void initData();

			DeclareRegFunction(shopInfo);
			DeclareRegFunction(buy);
			DeclareRegFunction(flush);
			DeclareRegFunction(boxRewardReq);
			DeclareRegFunction(getBoxRewardReq);

			KingdomWar::ShopDataPtr getShopData(int id);
			KingdomWar::ShopDataList getShopList(int nation);
			KingdomWar::BoxDataPtr getBoxData(int id);
			int getFlushCost(unsigned times) const;

		private:
			void loadFile();

		private:
			KingdomWar::ShopDataMap _shop_map;
			KingdomWar::BoxDataMap _box_map;

			KingdomWar::ShopDataList _shopRd[Kingdom::nation_num + 1];
			KingdomWar::ShopDataList _shopFix[Kingdom::nation_num + 1];
			unsigned _weight[Kingdom::nation_num + 1];
			std::vector<int> _flush_costs;
	};
}
