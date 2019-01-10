/*
	phantom is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	phantom is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with phantom.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CONSOLE_H_
#define CONSOLE_H_

namespace phantom {
	typedef std::function<void(const utils::StringVector &args)> ConsolePoc;
	typedef std::map<std::string, ConsolePoc> ConsolePocMap;

	class Console : public utils::Singleton<Console>, public utils::Runnable {
		friend class utils::Singleton<Console>;
	public:
		Console();
		~Console();

		bool Initialize();
		bool Exit();
		void Usage(const utils::StringVector &args);

		virtual void Run(utils::Thread *thread) override;
		void OpenWallet(const utils::StringVector &args);
		void CreateWallet(const utils::StringVector &args);
		void CloseWallet(const utils::StringVector &args);
		void RestoreWallet(const utils::StringVector &args);
		void GetBlockNumber(const utils::StringVector &args);
		void GetBalance(const utils::StringVector &args);
		void GetAddress(const utils::StringVector &args);
		void PayCoin(const utils::StringVector &args);
		void GetState(const utils::StringVector &args);
		void ShowKey(const utils::StringVector &args);
		void CmdExit(const utils::StringVector &args);

	private:
		utils::Thread *thread_ptr_;
		ConsolePocMap funcs_;

		PrivateKey *priv_key_;
		std::string keystore_path_;
		void CreateKestore(const utils::StringVector &args, std::string &private_key);
		PrivateKey *OpenKeystore( const std::string &path);
	};
}

#endif