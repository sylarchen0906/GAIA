#ifndef	 __GAIA_NETWORK_HTTPBASE_H__
#define	 __GAIA_NETWORK_HTTPBASE_H__

#include "gaia_type.h"
#include "gaia_assert.h"
#include "gaia_algo_string.h"
#include "gaia_ctn_string.h"
#include "gaia_ctn_charsstring.h"
#include "gaia_ctn_vector.h"

namespace GAIA
{
	namespace NETWORK
	{
		// Http code declaration here.
		static const GAIA::N32 HTTP_CODE_OK 					= 200;
		static const GAIA::N32 HTTP_CODE_BADREQUEST 			= 400;
		static const GAIA::N32 HTTP_CODE_UNAUTHORIZED 			= 401;
		static const GAIA::N32 HTTP_CODE_FORBIDDEN 				= 403;
		static const GAIA::N32 HTTP_CODE_NOTFOUND 				= 404;
		static const GAIA::N32 HTTP_CODE_INTERNALSERVERERROR 	= 500;
		static const GAIA::N32 HTTP_CODE_SERVERUNAVAILABLE 		= 503;
		static const GAIA::N32 HTTP_CODE_LIST[] = 
		{
			HTTP_CODE_OK,
			HTTP_CODE_BADREQUEST,
			HTTP_CODE_UNAUTHORIZED,
			HTTP_CODE_FORBIDDEN,
			HTTP_CODE_NOTFOUND,
			HTTP_CODE_INTERNALSERVERERROR,
			HTTP_CODE_SERVERUNAVAILABLE,
		};

		// Http head origin name declaration here.
		static const GAIA::CH* HTTP_HEADNAME_ACCEPTRANGES 		= "Accept-Ranges";
		static const GAIA::CH* HTTP_HEADNAME_CONNECTION 		= "Connection";
		static const GAIA::CH* HTTP_HEADNAME_CONTENTLENGTH 		= "Content-Length";
		static const GAIA::CH* HTTP_HEADNAME_CONTENTTYPE 		= "Content-Type";
		static const GAIA::CH* HTTP_HEADNAME_DATE 				= "Date";
		static const GAIA::CH* HTTP_HEADNAME_HOST 				= "Host";
		static const GAIA::CH* HTTP_HEADNAME_LASTMODIFIED 		= "Last-Modified";
		static const GAIA::CH* HTTP_HEADNAME_SERVER 			= "Server";
		static const GAIA::CH* HTTP_HEADNAME_SETCOOKIE 			= "Set-Cookie";
		static const GAIA::CH* HTTP_HEADNAME_USERAGENT 			= "User-Agent";
		static const GAIA::CH* HTTP_HEADNAME_LIST[] = 
		{
			HTTP_HEADNAME_ACCEPTRANGES,
			HTTP_HEADNAME_CONNECTION,
			HTTP_HEADNAME_CONTENTLENGTH,
			HTTP_HEADNAME_CONTENTTYPE,
			HTTP_HEADNAME_DATE,
			HTTP_HEADNAME_HOST,
			HTTP_HEADNAME_LASTMODIFIED,
			HTTP_HEADNAME_SERVER,
			HTTP_HEADNAME_SETCOOKIE,
			HTTP_HEADNAME_USERAGENT,
		};

		/*!
			@brief Http url management class.
		*/
		class HttpURL : public GAIA::Base
		{
		public:
			GINL HttpURL(){this->init();}
			GINL HttpURL(const HttpURL& src){this->init(); this->operator = (src);}
			GINL HttpURL(const GAIA::CH* psz){this->init(); this->operator = (psz);}
			GINL ~HttpURL(){}
			GINL GAIA::GVOID Reset(){m_url.clear(); this->clear_analyzed();}
			GINL GAIA::BL FromString(const GAIA::CH* psz){m_url = psz; this->clear_analyzed(); return GAIA::True;}
			GINL const GAIA::CH* ToString() const{return m_url.fptr();}
			GINL GAIA::CH* GetProtocal(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				return this->get_analyzed_node(m_protocal, psz, sMaxSize, pResultSize);
			}
			GINL GAIA::CH* GetHostName(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				return this->get_analyzed_node(m_hostname, psz, sMaxSize, pResultSize);
			}
			GINL GAIA::CH* GetPort(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				return this->get_analyzed_node(m_port, psz, sMaxSize, pResultSize);
			}
			GINL GAIA::CH* GetPath(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				return this->get_analyzed_node(m_path, psz, sMaxSize, pResultSize);
			}
			GINL GAIA::CH* GetFullQuery(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				return this->get_analyzed_node(m_fullquery, psz, sMaxSize, pResultSize);
			}
			GINL GAIA::BL GetQuery(GAIA::NUM sIndex, GAIA::CH* pszName, GAIA::CH* pszValue, GAIA::NUM sMaxNameSize = GINVALID, GAIA::NUM sMaxValueSize = GINVALID, GAIA::NUM* pNameResultSize = GNIL, GAIA::NUM* pValueResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GAIA::False;
				if(sIndex < 0 || sIndex >= m_queries.size())
					return GAIA::False;
				const Node& n = m_queries[sIndex];
				GAIA::CH* pszResultName = this->get_analyzed_node(n, pszName, sMaxNameSize, pNameResultSize);
				if(pszResultName == GNIL)
					return GAIA::False;
				GAIA::CH* pszResultValue = this->get_analyzed_node(n, pszValue, sMaxValueSize, pValueResultSize);
				return GAIA::True;
			}
			GINL GAIA::CH* GetQueryByName(const GAIA::CH* pszName, GAIA::CH* pszValue, GAIA::NUM sMaxValueSize, GAIA::NUM* pValueResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				GAST(m_queries.size() % 2 == 0);
				for(GAIA::NUM x = 0; x < m_queries.size(); x += 2)
				{
					const Node& nname = m_queries[x + 0];
					if(GAIA::ALGO::gstrcmp(nname.psz, pszName, nname.sLen) == 0)
					{
						const Node& nvalue = m_queries[x + 1];	
						return this->get_analyzed_node(nvalue, pszValue, sMaxValueSize, pValueResultSize);
					}
				}
				return GNIL;
			}
			GINL GAIA::CH* GetFragment(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				if(!GCCAST(HttpURL*)(this)->analyze())
					return GNIL;
				return this->get_analyzed_node(m_fragment, psz, sMaxSize, pResultSize);
			}
			GINL HttpURL& operator = (const HttpURL& src)
			{
				if(m_url == src.m_url)
					return *this;
				m_url = src.m_url;
				this->clear_analyzed();
				return *this;
			}
			GINL HttpURL& operator = (const GAIA::CH* psz)
			{
				if(m_url == psz)
					return *this;
				this->FromString(psz);
				return *this;
			}
			GINL operator const GAIA::CH*(){return m_url.fptr();}
			GINL GAIA::N32 compare(const GAIA::CH* psz) const{return m_url.compare(psz);}
			GINL GAIA::N32 compare(const HttpURL& src) const{return m_url.compare(src.m_url);}
			GCLASS_COMPARE_BYCOMPARE(HttpURL)

		private:
			class Node : public GAIA::Base
			{
			public:
				GINL GAIA::GVOID reset(){psz = GNIL; sLen = 0;}
			public:
				const GAIA::CH* psz;
				GAIA::NUM sLen;
			};

		private:
			GINL GAIA::GVOID init(){this->clear_analyzed();}
			GINL GAIA::GVOID clear_analyzed()
			{
				m_protocal.reset();
				m_hostname.reset();
				m_port.reset();
				m_path.reset();
				m_fullquery.reset();
				m_fragment.reset();
				m_queries.clear();
				m_bAnalyzed = GAIA::False;
			}
			GINL GAIA::BL analyze()
			{
				if(m_bAnalyzed)
					return GAIA::True;
				if(m_url.empty())
					return GAIA::False;

				// Local variable declarations.
				GAIA::NUM sProtocalBegin, sProtocalEnd;
				GAIA::NUM sHostNameBegin, sHostNameEnd;
				GAIA::NUM sPortBegin, sPortEnd;
				GAIA::NUM sPathBegin, sPathEnd;
				GAIA::NUM sFullQueryBegin, sFullQueryEnd;
				GAIA::NUM sFragBegin, sFragEnd;

				GAIA::NUM sUrlSize = m_url.size();

				// Partition analyze.
				{
					// Protocal.
					sProtocalBegin = 0;
					sProtocalEnd = m_url.find("://");
					if(sProtocalEnd == GINVALID)
						sProtocalEnd = 0;

					// HostName.
					sHostNameBegin = ((sProtocalEnd == 0) ? (0) : (sProtocalEnd + sizeof("://") - 1));

					// Port.
					sPortBegin = m_url.find(':', sHostNameBegin);
					sPortEnd = GINVALID;
					if(sPortBegin == GINVALID)
					{
						sHostNameEnd = m_url.find('/', sHostNameBegin);
						if(sHostNameEnd == GINVALID)
							sHostNameEnd = sUrlSize;
					}
					else
					{
						sHostNameEnd = sPortBegin;
						sPortBegin++;
						if(sPortBegin < sUrlSize)
						{
							sPortEnd = m_url.find('/', sPortBegin);
							if(sPortEnd == GINVALID)
								sPortEnd = sUrlSize;
						}
					}

					// Path.
					sPathBegin = ((sPortEnd != GINVALID) ? sPortEnd : sHostNameEnd);
					sPathEnd = m_url.rfind('/', sPathBegin + 1);
					if(sPathEnd == GINVALID)
						sPathEnd = sUrlSize;

					// Full query.
					sFullQueryBegin = sPathEnd + 1;
					sFullQueryEnd = m_url.rfind('#', sFullQueryBegin + 1);
					if(sFullQueryEnd == GINVALID)
						sFullQueryEnd = sUrlSize;

					// Fragment.
					sFragBegin = sFullQueryEnd + 1;
					sFragEnd = sUrlSize;
				}

				// Checkup.
				{
					if(sProtocalBegin >= 0 && sProtocalEnd < sUrlSize && sProtocalBegin < sProtocalEnd)
					{
						m_protocal.psz = m_url.fptr() + sProtocalBegin;
						m_protocal.sLen = sProtocalEnd - sProtocalBegin;
					}

					if(sHostNameBegin >= 0 && sHostNameEnd < sUrlSize && sHostNameBegin < sHostNameEnd)
					{
						m_hostname.psz = m_url.fptr() + sHostNameBegin;
						m_hostname.sLen = sHostNameEnd - sHostNameBegin;
					}

					if(sPortBegin >= 0 && sPortEnd < sUrlSize && sPortBegin < sPortEnd)
					{
						m_port.psz = m_url.fptr() + sPortBegin;
						m_port.sLen = sPortEnd - sPortBegin;
					}

					if(sPathBegin >= 0 && sPathEnd < sUrlSize && sPathBegin < sPathEnd)
					{
						m_path.psz = m_url.fptr() + sPathBegin;
						m_path.sLen = sPathEnd - sPathBegin;
					}

					if(sFullQueryBegin >= 0 && sFullQueryEnd < sUrlSize && sFullQueryBegin < sFullQueryEnd)
					{
						m_fullquery.psz = m_url.fptr() + sFullQueryBegin;
						m_fullquery.sLen = sFullQueryEnd - sFullQueryBegin;
					}

					if(sFragBegin >= 0 && sFragEnd < sUrlSize && sFragBegin < sFragEnd)
					{
						m_fragment.psz = m_url.fptr() + sFragBegin;
						m_fragment.sLen = sFragEnd - sFragBegin;
					}
				}

				// Full query analyze to list.
				{

				}

				m_bAnalyzed = GAIA::True;
				return GAIA::True;
			}
			GINL GAIA::CH* get_analyzed_node(const Node& n, GAIA::CH* psz, GAIA::NUM sMaxSize, GAIA::NUM* pResultSize) const
			{
				if(psz == GNIL)
				{
					GAST(sMaxSize == GINVALID);
					if(*pResultSize != GNIL)
						*pResultSize = n.sLen;
					return (GAIA::CH*)n.psz;
				}
				else
				{
					if(n.sLen >= sMaxSize)
						return GNIL;
					if(*pResultSize != GNIL)
						*pResultSize = n.sLen;
					GAIA::ALGO::gstrcpy(psz, n.psz, n.sLen);
					return psz;
				}
			}

		private:
			GAIA::CTN::AString m_url;
			Node m_protocal;
			Node m_hostname;
			Node m_port;
			Node m_path;
			Node m_fullquery;
			Node m_fragment;
			GAIA::CTN::Vector<Node> m_queries;
			GAIA::BL m_bAnalyzed;
		};

		/*!
			@brief Http request heads management class.
		*/
		class HttpHead : public GAIA::Base
		{
		public:
			GINL HttpHead(){this->init();}
			GINL HttpHead(const HttpHead& src){this->init(); this->operator = (src);}
			GINL HttpHead(const GAIA::CH* psz){this->init(); this->operator = (psz);}
			GINL ~HttpHead(){this->Reset();}
			GINL GAIA::BL FromString(const GAIA::CH* psz)
			{
				this->Reset();
				if(GAIA::ALGO::gstremp(psz))
					return GAIA::True;
				const GAIA::CH* p = psz;
				const GAIA::CH* pLast = p;
				GAIA::CTN::ACharsString strName;
				GAIA::CTN::ACharsString strValue;
				while(*p != '\0')
				{
					if(*p == ':') // Name end.
					{
						if(!strName.empty() || p - pLast == 0)
						{
							this->Reset();
							return GAIA::False;
						}
						strName.assign(pLast, p - pLast);
						p += 1;
						pLast = p;
					}
					else if(*p == '\r' && *(p + 1) == '\n') // Value end.
					{
						if(strName.empty() || p - pLast == 0)
						{
							this->Reset();
							return GAIA::False;
						}
						strValue.assign(pLast, p - pLast);
						this->Set(strName.fptr(), strValue.fptr());
						strName.clear();
						strValue.clear();
						p += 2;
						pLast = p;
					}
				}
				if(p != pLast)
				{
					if(strName.empty() || p - pLast == 0)
					{
						this->Reset();
						return GAIA::False;
					}
					strValue.assign(pLast, p - pLast);
					this->Set(strName.fptr(), strValue.fptr());
				}
				return GAIA::True;
			}
			GINL GAIA::CH* ToString(GAIA::CH* psz, GAIA::NUM sMaxSize = GINVALID, GAIA::NUM* pResultSize = GNIL) const
			{
				GAST(!GAIA::ALGO::gstremp(psz));
				GAIA::CH* p = psz;
				GAIA::NUM sSize = 0;
				for(GAIA::NUM x = 0; x < m_nodes.size(); ++x)
				{
					const Node& n = m_nodes[x];
					GAIA::NUM sNeedSize = GAIA::ALGO::gstrlen(n.pszName) + GAIA::ALGO::gstrlen(n.pszValue) + sizeof(":\r\n\0") - 1;
					if(sSize + sNeedSize > sMaxSize)
						break;
					sSize += sNeedSize - 1;
					p = GAIA::ALGO::gstradd_notend(p, n.pszName); *p++ = ':';
					p = GAIA::ALGO::gstradd_notend(p, n.pszValue); *p++ = '\r'; *p++ = '\n';
				}
				*p++ = '\0';
				if(pResultSize != GNIL)
					*pResultSize = sSize;
				return psz;
			}
			GINL GAIA::BL Set(const GAIA::CH* pszName, const GAIA::CH* pszValue, GAIA::BL bNotExistFailed = GAIA::False)
			{
				GAST(!GAIA::ALGO::gstremp(pszName));
				GAST(pszValue != GNIL);
				if(m_bSorted)
				{
					Node finder;
					finder.pszName = pszName;
					GAIA::NUM sFinded = m_nodes.binary_search(finder);
					if(sFinded == GINVALID)
					{
						if(bNotExistFailed)
							return GAIA::False;
						Node n;
						n.pszName = this->requestname(pszName);
						n.pszValue = this->requestvalue(pszValue);
						m_nodes.push_back(n);
						m_bSorted = GAIA::False;
					}
					else
					{
						Node& n = m_nodes[sFinded];
						this->releasevalue(n.pszValue);
						n.pszValue = this->requestvalue(pszValue);
					}
				}
				else
				{
					GAIA::BL bExist = GAIA::False;
					for(GAIA::NUM x = 0; x < m_nodes.size(); ++x)
					{
						Node& n = m_nodes[x];
						if(GAIA::ALGO::gstrequal(n.pszName, pszName))
						{
							this->releasevalue(n.pszValue);
							n.pszValue = this->requestvalue(pszValue);
							bExist = GAIA::True;
							break;
						}
					}
					if(!bExist)
					{
						if(bNotExistFailed)
							return GAIA::False;
						Node n;
						n.pszName = this->requestname(pszName);
						n.pszValue = this->requestvalue(pszValue);
						m_nodes.push_back(n);
					}
				}
				return GAIA::True;
			}
			GINL const GAIA::CH* Get(const GAIA::CH* pszName) const
			{
				GAST(!GAIA::ALGO::gstremp(pszName));
				GCCAST(HttpHead*)(this)->sortnodes();
				Node finder;
				finder.pszName = pszName;
				GAIA::NUM sFinded = m_nodes.binary_search(finder);
				if(sFinded == GINVALID)
					return GNIL;
				return m_nodes[sFinded].pszName;
			}
			GINL GAIA::BL Exist(const GAIA::CH* pszName) const
			{
				GAST(!GAIA::ALGO::gstremp(pszName));
				return this->Get(pszName) != GNIL;
			}
			GINL GAIA::BL Delete(const GAIA::CH* pszName)
			{
				GAST(!GAIA::ALGO::gstremp(pszName));
				if(m_bSorted)
				{
					Node finder;
					finder.pszName = pszName;
					GAIA::NUM sFinded = m_nodes.binary_search(finder);
					if(sFinded == GINVALID)
						return GAIA::False;
					this->releasevalue(m_nodes[sFinded].pszValue);
					return m_nodes.erase(sFinded);
				}
				else
				{
					for(GAIA::NUM x = 0; x < m_nodes.size(); ++x)
					{
						const Node& n = m_nodes[x];
						if(GAIA::ALGO::gstrequal(n.pszName, pszName))
						{
							this->releasevalue(n.pszValue);
							return m_nodes.erase(x);
						}
					}
					return GAIA::False;
				}
			}
			GINL GAIA::GVOID Reset()
			{
				for(GAIA::NUM x = 0; x < m_nodes.size(); ++x)
				{
					Node& n = m_nodes[x];
					this->releasevalue(n.pszValue);
				}
				m_nodes.clear();
				m_bSorted = GAIA::True;
			}
			GINL GAIA::NUM Count() const{return m_nodes.size();}
			GINL const GAIA::CH* GetName(GAIA::NUM sIndex) const
			{
				GAST(sIndex >= 0 && sIndex < m_nodes.size());
				if(sIndex < 0 || sIndex >= m_nodes.size())
					return GNIL;
				return m_nodes[sIndex].pszName;
			}
			GINL const GAIA::CH* GetValue(GAIA::NUM sIndex) const
			{
				GAST(sIndex >= 0 && sIndex < m_nodes.size());
				if(sIndex < 0 || sIndex >= m_nodes.size())
					return GNIL;
				return m_nodes[sIndex].pszValue;
			}
			GINL GAIA::BL GetNameAndValue(GAIA::NUM sIndex, GAIA::CH** ppName, GAIA::CH** ppValue) const
			{
				GAST(sIndex >= 0 && sIndex < m_nodes.size());
				if(sIndex < 0 || sIndex >= m_nodes.size())
					return GAIA::False;
				const Node& n = m_nodes[sIndex];
				*ppName = (GAIA::CH*)n.pszName;
				*ppValue = (GAIA::CH*)n.pszValue;
				return GAIA::True;
			}
			GINL HttpHead& operator = (const HttpHead& src)
			{
				this->Reset();
				for(GAIA::NUM x = 0; x < src.m_nodes.size(); ++x)
				{
					const Node& n = src.m_nodes[x];
					Node nn;
					nn.pszName = n.pszName;
					nn.pszValue = this->requestvalue(n.pszValue);
					m_nodes.push_back(nn);
				}
				m_bSorted = src.m_bSorted;
				return *this;
			}
			GINL HttpHead& operator = (const GAIA::CH* psz)
			{
				this->FromString(psz);
				return *this;
			}
			GINL GAIA::N32 compare(const GAIA::CH* psz) const
			{
				HttpHead src = psz;
				return this->compare(src);
			}
			GINL GAIA::N32 compare(const HttpHead& src) const
			{
				GCCAST(HttpHead*)(this)->sortnodes();
				GCCAST(HttpHead*)(&src)->sortnodes();
				if(m_nodes.size() < src.m_nodes.size())
					return -1;
				else if(m_nodes.size() > src.m_nodes.size())
					return +1;
				for(GAIA::NUM x = 0; x < m_nodes.size(); ++x)
				{
					const Node& n1 = m_nodes[x];
					const Node& n2 = src.m_nodes[x];
					GAIA::N32 nCmp = n1.compare(n2);
					if(nCmp != 0)
						return nCmp;
				}
				return 0;
			}
			GCLASS_COMPARE_BYCOMPARE(HttpHead)

		private:
			class Node : public GAIA::Base
			{
			public:
				GINL GAIA::N32 compare(const Node& src) const{return GAIA::ALGO::gstrcmp(pszName, src.pszName);}
				GCLASS_COMPARE_BYCOMPARE(Node)
			public:
				const GAIA::CH* pszName;
				const GAIA::CH* pszValue;
			};

		private:
			GINL GAIA::GVOID init(){m_bSorted = GAIA::False;}
			GINL GAIA::GVOID sortnodes()
			{
				if(m_bSorted)
					return;
				m_nodes.sort();
				m_bSorted = GAIA::True;
			}
			GINL const GAIA::CH* requestname(const GAIA::CH* pszName);
			GINL const GAIA::CH* requestvalue(const GAIA::CH* pszValue);
			GINL GAIA::BL releasevalue(const GAIA::CH* pszValue);

		private:
			GAIA::CTN::Vector<Node> m_nodes;
			GAIA::BL m_bSorted;
		};
	}
}

#endif
