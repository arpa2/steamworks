'use strict';

angular.
  module('crankApp').
  component('crankIssuerView', {
    templateUrl: 'app/issuer/crank-issuer-view.template.html',
    controller: function CrankIssuerListController($http, $routeParams, config) {
      this.status = false;
      this.issuerdn = $routeParams.issuerdn;
      this.issuerdata = undefined;
      this._issuerdata = undefined;
      
      var self = this;
      
      $http.post(config.basecgi, {
        verb: 'search',
        base: this.issuerdn,
        filter: 'objectClass=tlsPoolTrustedIssuer'}).
      then(
        function(response) {
          self.issuerdata = response.data[self.issuerdn];
          delete self.issuerdata.objectClass;
          self._issuerdata = angular.copy(self.issuerdata);
          self.status = true;
          
          // While editing, DN (last one) remains readonly.
          var l = document.getElementById('issuerview').getElementsByTagName("input");
          var e;
          for(e=0; e<l.length-1; e++) {
            l[e].removeAttribute("readonly");
          }
        },
        function(response) {
          self.issuerdata = undefined;
          self.status = false;
      });

      this.do_save = function() {
        var d = {};
        var k;
        for (k in self._issuerdata) {
          if (self.issuerdata[k] != self._issuerdata[k]) {
            d[k] = self.issuerdata[k];
          }
        }
        d.dn = self._issuerdata.dn;
        $http.post(config.basecgi, {
          verb: 'update',
          values: [ d ]});
        window.location = '#!/issuers';
      }
    }
  });
