'use strict';

angular.
  module('crankApp').
  component('crankIssuerAdd', {
    templateUrl: 'app/issuer/crank-issuer-view.template.html',
    controller: function CrankIssuerAddController($http, $routeParams, config) {
      this.status = true;
      this.issuerdn = undefined;
      this.issuerdata = {};
      this.basedn = config.basedn;

      // While adding, DN is writable, too.
      var l = document.getElementById('issuerview').getElementsByTagName("input");
      var e;
      for(e=0; e<l.length; e++) {
        l[e].removeAttribute("readonly");
      }

      this.do_save = function(self) {
        if (self.issuerdn && self.issuerdata) {
          self.issuerdata.dn = self.issuerdn;
	  self.issuerdata.objectClass = 'tlsPoolTrustedIssuer';
          $http.post(config.basecgi, {
            verb: 'add',
            values: [ self.issuerdata ]});
          window.location = '#!/issuers';
        }
      }
    }
  });
