'use strict';

angular.
  module('crankApp').
  component('crankStatus', {
    templateUrl: 'app/status/crank-status.template.html',
    controller: function CrankStatusController($http, config) {
      this.status = false;
      this.serverstatus = "Checking ..";
      
      var self = this;
      $http.post(config.basecgi, {verb: 'serverstatus'}).then(
        function(response) {
          self.serverstatus = response.data.message;
          self.status = (response.data._status == 1);
        },
        function(response) {
          self.serverstatus = "Error contacting Crank";
          self.status = false;
      })
    }
  });
