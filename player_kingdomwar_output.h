#pragma once

#include "auto_base.h"

namespace gg
{
	class playerKingdomWarOutput
		: public _auto_player
	{
		public:
			playerKingdomWarOutput(playerData* const own);

			void loadDB();
			virtual bool _auto_save();
			virtual void _auto_update();
			
			void update();
			int getOutput(int id);

		private:
			void init();
			inline bool outputEmpty() const;
			void doGetOutput(int type, int val);

		private:
			STDVECTOR(int, Output);
			Output _output;
			/*Output _silver;
			Output _fame;
			Output _merit;*/

			unsigned _next_output_time;
	};
}
