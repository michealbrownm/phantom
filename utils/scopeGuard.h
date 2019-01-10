#ifndef SCOPE_GUARD_H
#define SCOPE_GUARD_H

#include "logger.h"

namespace phantom
{
    template<class T>
    T& getRef(T& t){ return t; }
    
    template<class T>
    T& getRef(T* t){ return *t; }

    class ScopeGuardBase
    {
        protected:
            mutable bool dismissed_;

        public:
            void Dismiss() const throw(){ dismissed_ = true; }
    
        protected:
            ScopeGuardBase():dismissed_(false){}
            ScopeGuardBase(const ScopeGuardBase& other): dismissed_(other.dismissed_) { other.Dismiss(); }
            ScopeGuardBase& operator=(const ScopeGuardBase& other)
            {
                dismissed_ = other.dismissed_;
                other.Dismiss();
            }
            ~ScopeGuardBase(){} //note: no virtual

            virtual void Execute() = 0;

            template<typename OBJECT>
            static void SafeExecute(OBJECT& obj)
            {
				if (!obj.dismissed_)
				{
					try{ obj.Execute(); }
					catch (std::exception& e){ LOG_ERROR(e.what()); }
				}
            }
    };

    typedef const ScopeGuardBase& ScopeGuard;
    
    //template<typename FUN, typename PARM>
    //class FunScopeGuardParm1: public ScopeGuardBase
    //{
    //    private:
    //        FUN fun_;
    //        const PARM parm_;
    //
    //    public:
    //        FunScopeGuardParm1() = delete;
    //        FunScopeGuardParm1(FUN fun, PARM& parm): fun_(fun), parm_(parm){}
    //        ~FunScopeGuardParm1(){ SafeExecute(*this); } //can't be written in destructor func of base class

    //        void Execute(){ fun_(); }
    //};

    //template<typename FUN, typename PARM>
    //inline FunScopeGuardParm1<FUN, PARM> MakeGuard(FUN fun, PARM parm)
    //{
    //    return FunScopeGuardParm1<FUN, PARM>(fun, parm);
    //}

    template<class OBJ, typename M_FUN>
    class ObjScopeGuardParm0: public ScopeGuardBase
    {
        private:
            OBJ& obj_;
            M_FUN fun_;

        public:
            ObjScopeGuardParm0() = delete;
            ObjScopeGuardParm0(OBJ& obj, M_FUN fun): obj_(obj), fun_(fun){}
            ~ObjScopeGuardParm0(){ SafeExecute(*this); } //can't be written in destructor func of base class

            void Execute(){ (getRef(obj_).*fun_)(); }
    };

    template<class OBJ, typename M_FUN>
    inline ObjScopeGuardParm0<OBJ, M_FUN> MakeGuard(OBJ& obj, M_FUN fun)
    {
        return ObjScopeGuardParm0<OBJ, M_FUN>(obj, fun);
    }

    template<class OBJ, typename M_FUN, typename PARM>
    class ObjScopeGuardParm1: public ScopeGuardBase
    {
        private:
            OBJ& obj_;
            M_FUN fun_;
            PARM parm_;

        public:
            ObjScopeGuardParm1() = delete;
            ObjScopeGuardParm1(OBJ& obj, M_FUN fun, PARM& parm): obj_(obj), fun_(fun), parm_(parm){}
            ~ObjScopeGuardParm1(){ SafeExecute(*this); } //can't be written in destructor func of base class

            void Execute(){ (getRef(obj_).*fun_)(parm_); }
    };

    template<class OBJ, typename M_FUN, typename PARM>
    inline ObjScopeGuardParm1<OBJ, M_FUN, PARM> MakeGuard(OBJ& obj, M_FUN fun, PARM& parm)
    {
        return ObjScopeGuardParm1<OBJ, M_FUN, PARM>(obj, fun, parm);
    }


    template<class OBJ, typename M_FUN, typename PARM0, typename PARM1>
    class ObjScopeGuardParm2: public ScopeGuardBase
    {
        private:
            OBJ& obj_;
            M_FUN fun_;
            PARM0 parm0_;
            PARM1 parm1_;

        public:
            ObjScopeGuardParm2() = delete;
            ObjScopeGuardParm2(OBJ& obj, M_FUN fun, PARM0& parm0, PARM1& parm1): obj_(obj), fun_(fun), parm0_(parm0), parm1_(parm1){}
            ~ObjScopeGuardParm2(){ SafeExecute(*this); } //can't be written in destructor func of base class

            void Execute(){ (getRef(obj_).*fun_)(parm0_, parm1_); }
    };

    template<class OBJ, typename M_FUN, typename PARM0, typename PARM1>
    inline ObjScopeGuardParm2<OBJ, M_FUN, PARM0, PARM1> MakeGuard(OBJ& obj, M_FUN fun, PARM0& parm0, PARM1& parm1)
    {
        return ObjScopeGuardParm2<OBJ, M_FUN, PARM0, PARM1>(obj, fun, parm0, parm1);
    }
}

#endif //SCOPE_GUARD_H
