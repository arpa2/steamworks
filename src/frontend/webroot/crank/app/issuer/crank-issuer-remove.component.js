'use strict';

angular.
  module('crankApp').
  component('crankIssuerRemove', {
    templateUrl: 'app/issuer/crank-issuer-remove.template.html',
    controller: function CrankIssuerRemoveController($http, $routeParams, config) {
      this.status = false;
      this.issuerdn = $routeParams.issuerdn;
      
      var self = this;
      this.do_rm = function() { 
        $http.post(config.basecgi, {
          verb: 'delete',
          dn: self.issuerdn });
        window.location = '#!/issuers';
      }
    }
  });
