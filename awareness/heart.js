angular.module('sqrt', ["kendo.directives"])
    .controller('MainController', function( $scope){
        $scope.model = window.model;
        setInterval(function(){
            $scope.model.heartRate = window.model.heartRate;
            $scope.$apply()
        }, 100);
    })
    .controller('HeartController', function( $scope){

    });