'use strict';

angular.
  module('crankApp').
  component('crankCertificateList', {
    templateUrl: 'app/certificate/crank-certificate-list.template.html',
    controller: function CrankCertificateListController($http, config) {
      this.status = false;
      this.certificates = [];

      var self = this;
      $http.post(config.basecgi, {
        verb: 'search',
        base: 'dc=example,dc=com',
        filter: 'objectClass=pkcs11PrivateKeyObject'}).
      then(
        function(response) {
          self.certificates = response.data;
          self.status = true;
        },
        function(response) {
          self.certificates = [];
          self.status = false;
      })

    }
  });
