/*
 MPServerResolver.hxx - mpserver names lookup via DNS
 Written and copyright by Torsten Dreyer - November 2016

 This file is part of FlightGear.

 FlightGear is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 FlightGear is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with FlightGear.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __FG_MPSERVERRESOLVER_HXX
#define __FG_MPSERVERRESOLVER_HXX

#include <string>
#include <simgear/props/props.hxx>

class MPServerResolver {
public:
  MPServerResolver();
  virtual ~MPServerResolver();
  void run();

  /**
   * Set the target property where the server-list gets stored
   *
   * \param value the property node to use as a target
   */
  void setTarget( SGPropertyNode_ptr value ) { _targetNode = value; }

  /**
   * Set the dns domain name to query. This could be either a full qualified name including the
   * service and the protocol like _fgms._udp.flightgear.org or just the domain name like
   * flightgear.org. Use setService() and setProtocol() in the latter case.
   *
   * \param value the dnsname to use for the query.
   */
  void setDnsName( const std::string & value ) { _dnsName = value; }

  /** Set the service name to use for the query. Don't add the underscore, this gets added
   * automatically. This builds the fully qualified DNS name to query, together with
   * setProtocol() and setDnsName().
   *
   * \param value the service name to use for the query sans the leading underscore
   */
  void setService( const std::string & value ) { _service = value; }

  /** Set the protocol name to use for the query. Don't add the underscore, this gets added
    * automatically. This builds the fully qualified DNS name to query, together with
    * setService() and setDnsName().
    *
    * \param value the protocol name to use for the query sans the leading underscore
    */
  void setProtocol( const std::string & value ) { _protocol = value; }

  /** Handler to be called if the resolver process finishes with success. Does nothing by
   * default and should be overridden be the user.
   */
  virtual void onSuccess() {};

  /** Handler to be called if the resolver process terminates with an error. Does nothing by
    * default and should be overridden be the user.
    */
  virtual void onFailure() {};

private:
  class MPServerResolver_priv;
  std::string _dnsName;
  std::string _service;
  std::string _protocol;
  SGPropertyNode_ptr _targetNode;
  MPServerResolver_priv * _priv;
};

#endif // __FG_MPSERVERRESOLVER_HXX
