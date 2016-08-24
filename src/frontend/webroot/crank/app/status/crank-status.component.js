'use strict';

angular.
  module('crankApp').
  component('crankStatus', {
    templateUrl: 'app/status/crank-status.template.html',
    controller: function CrankStatusController($http, config, $location) {
      this.status = false;
      this.statuscode = -1;
      this.serverstatus = "Checking ..";
      this.hosturi = null;
      this.hostmaster = null;
      this.hostsecret = null;

      var self = this;

      this.do_status = function(self, redirect) {
        $http.post(config.basecgi, {verb: 'serverstatus'}).then(
          function(response) {
            self.serverstatus = response.data.message;
            self.statuscode = response.data._status;
            self.status = (response.data._status == 1);
            if (self.status && redirect)
            {
              $location.url(redirect);
            }
          },
          function(response) {
            self.serverstatus = "Error contacting Crank";
            self.statuscode = -1;
            self.status = false;
        });
      }

      this.do_connect = function(self) {
      $http.post(config.basecgi,
        {verb: 'connect', uri: self.hosturi, user: self.hostmaster, password: self.hostsecret }).then(
        function(response) { self.do_status(self, "/issuers"); },
        function(response) { self.do_status(self); });
      }

      this.do_status(this);
    }
  });
