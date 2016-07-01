#pragma once

#include "auto_base.h"
#include "mongoDB.h"

namespace gg
{
	class playerMan;
	SHAREPTR(playerMan, playerManPtr);
	STDVECTOR(playerManPtr, ManList);

	class playerKingdomWarSGFM
		: public _auto_player
	{
		public:
			playerKingdomWarSGFM(int army_id, playerData* const own);

			void load(const mongo::BSONObj& obj);
			void getInfo(qValue& q);

			int setFormation(int fm_id, const int fm[9]);
			const ManList& manList() const { return _man_list; }
			int fmId() const { return _fm_id; }
			int bv() const { return _power; }
			void useHpItem();

		private:
			virtual void _auto_update();
			virtual bool _auto_save(); 
			void recalFM();

		private:
			const int _army_id;
			int _fm_id;
			int _power;
			ManList _man_list;
	};

	class playerKingdomWarFM
		: public _auto_player
	{
		public:
			playerKingdomWarFM(playerData* const own);

			void loadDB();
			void update();

			const ManList& getFM(int army_id) const { return _fm_list[army_id]->manList(); }
			int getFMId(int army_id) const { return _fm_list[army_id]->fmId(); }
			int getBV(int army_id) const { return _fm_list[army_id]->bv(); }
			int setFormation(int army_id, int fm_id, const int fm[9]);
			bool manUsed(int army_id, int man_id) const;

		private:
			virtual bool _auto_save();
			virtual void _auto_update();

		private:
			SHAREPTR(playerKingdomWarSGFM, FMPtr);
			STDVECTOR(FMPtr, FMVec);
			FMVec _fm_list;
	};
}
