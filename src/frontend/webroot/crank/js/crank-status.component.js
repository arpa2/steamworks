'use strict';

angular.
  module('crankApp').
  component('crankStatus', {
    templateUrl: 'tmpl/crank-status.template.html',
    controller: function CrankStatusController($http) {
      this.status = 'Not connected';
      this.serverstatus = undefined;
      
      var self = this;
      $http.post('/cgi-bin/', {verb: 'serverinfo'}).then(function(response) { self.serverstatus = response.data; });
    }
  });
