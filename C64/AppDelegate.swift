/*
 AppDelegate.swift
 Copyright (C) 2019 Dieter Baron
 
 This file is part of C64, a Commodore 64 emulator for iOS, based on VICE.
 The authors can be contacted at <c64@spiderlab.at>
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 02111-1307  USA.
*/

import UIKit
import CoreData

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?


    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplication.LaunchOptionsKey: Any]?) -> Bool {
        // Override point for customization after application launch.

        return true
    }

    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
        // TODO: pause emulation
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
        // TODO: save emulation state
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
        updateLibrary()
    }

    func applicationWillTerminate(_ application: UIApplication) {
        // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
        // Saves changes in the application's managed object context before the application terminates.
        self.saveContext()
    }
    
    func application(_ app: UIApplication, open url: URL, options: [UIApplication.OpenURLOptionsKey : Any] = [:]) -> Bool {
        do {
            let directoryURL = AppDelegate.inboxURL
            try ensureDirectory(directoryURL)
            
            // TODO: check that media item can be created from file
            
            guard url.startAccessingSecurityScopedResource() else { return false }
            
            let fileCoordinator = NSFileCoordinator(filePresenter: nil)

            var error: NSError?
            
            var ok = true
            
            fileCoordinator.coordinate(readingItemAt: url, options: .withoutChanges, error: &error) { coordinatedUrl in
                do {
                    let fileURL = try uniqeName(directory: directoryURL, name: url.lastPathComponent, pathExtension: url.pathExtension)
                    try FileManager.default.copyItem(at: url, to: fileURL)
                }
                catch {
                    print("can't open \(url): \(error.localizedDescription)")
                    ok = false
                }
            }
            url.stopAccessingSecurityScopedResource()

            if ok && viceThread == nil {
                if let tabBarController = window?.rootViewController as? TabBarViewController {
                    tabBarController.selectedTab = .inbox
                }
            }
            
            return ok
        }
        catch {
            url.stopAccessingSecurityScopedResource()
            print("can't open \(url): \(error.localizedDescription)")
            return false
        }
    }
    
    func application(_ application: UIApplication, shouldRestoreApplicationState coder: NSCoder) -> Bool {
        return true
    }
    
    func application(_ application: UIApplication, shouldSaveApplicationState coder: NSCoder) -> Bool {
        return true
    }
    
    // MARK: - Globals
    
    static var documentURL: URL {
        guard let documentDirectoryPath = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true).first else { fatalError("no document directory found") }
        
        return URL(fileURLWithPath: documentDirectoryPath)
    }
    
    static var applicationSupportURL: URL {
        guard let documentDirectoryPath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first else { fatalError("no ApplicationSupport directory found") }
        
        return URL(fileURLWithPath: documentDirectoryPath)
    }

    static var inboxName: String {
        return "Unsorted"
    }
    
    static var exporedName: String {
        return "Exported"
    }
    
    static var inboxURL: URL {
        return documentURL.appendingPathComponent(inboxName)
    }
    
    static var exportedURL: URL {
        return documentURL.appendingPathComponent(exporedName)
    }
    
    static var libraryURL: URL {
        return applicationSupportURL.appendingPathComponent("Library")
    }
    
    static var biosURL: URL {
        return applicationSupportURL.appendingPathComponent("BIOS")
    }
    
    static var toolsURL: URL {
        return applicationSupportURL.appendingPathComponent("Tools")
    }
    
    static var viceDataURL: URL {
        return Bundle.main.resourceURL!.appendingPathComponent("vice")
    }

    // MARK: - Core Data stack

    lazy var persistentContainer: NSPersistentContainer = {
        /*
         The persistent container for the application. This implementation
         creates and returns a container, having loaded the store for the
         application to it. This property is optional since there are legitimate
         error conditions that could cause the creation of the store to fail.
        */
        let container = NSPersistentContainer(name: "C64")
        container.loadPersistentStores(completionHandler: { (storeDescription, error) in
            if let error = error as NSError? {
                // Replace this implementation with code to handle the error appropriately.
                // fatalError() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
                 
                /*
                 Typical reasons for an error here include:
                 * The parent directory does not exist, cannot be created, or disallows writing.
                 * The persistent store is not accessible, due to permissions or data protection when the device is locked.
                 * The device is out of space.
                 * The store could not be migrated to the current model version.
                 Check the error message to determine what the actual problem was.
                 */
                fatalError("Unresolved error \(error), \(error.userInfo)")
            }
        })
        return container
    }()

    // MARK: - Core Data Saving support

    func saveContext () {
        let context = persistentContainer.viewContext
        if context.hasChanges {
            do {
                try context.save()
            } catch {
                // Replace this implementation with code to handle the error appropriately.
                // fatalError() causes the application to generate a crash log and terminate. You should not use this function in a shipping application, although it may be useful during development.
                let nserror = error as NSError
                fatalError("Unresolved error \(nserror), \(nserror.userInfo)")
            }
        }
    }

    
    @objc func mergeContextChanges(_ notification: Notification) {
        persistentContainer.viewContext.mergeChanges(fromContextDidSave: notification)
    }

    // MARK: - Utility functions
    
    private var isUpdateLibraryRunning = false
    func updateLibrary() {
        guard !isUpdateLibraryRunning else { return }
        isUpdateLibraryRunning = true
        let context = persistentContainer.newBackgroundContext()
        NotificationCenter.default.addObserver(self, selector: #selector(mergeContextChanges(_:)), name: NSNotification.Name.NSManagedObjectContextDidSave, object: context)

        context.perform {
            self.importDocumentsDirectory(context: context)
            self.deleteExpiredGames(context: context)

            DispatchQueue.main.async {
                NotificationCenter.default.removeObserver(self, name: NSNotification.Name.NSManagedObjectContextDidSave, object: context)
                self.isUpdateLibraryRunning = false
            }
        }
    }
    
    func importDocumentsDirectory(context: NSManagedObjectContext) {
        do {
            let libraryURL = AppDelegate.libraryURL
            let documentURL = AppDelegate.documentURL
            let fileManager = FileManager.default
            
            let ignoredDirectories: Set<String> = [
                "Inbox",
                AppDelegate.exporedName,
                AppDelegate.inboxName
            ]
            
            for directory in try fileManager.contentsOfDirectory(at: documentURL, includingPropertiesForKeys: [.isDirectoryKey], options: [.skipsPackageDescendants, .skipsHiddenFiles]) {
                let isDirectory = try directory.resourceValues(forKeys: [.isDirectoryKey]).isDirectory ?? true
                let name = directory.lastPathComponent
                if !isDirectory || ignoredDirectories.contains(name) {
                    continue
                }
                print("importing \(name)")
                let targetUrl = try uniqeName(directory: libraryURL, name: name, pathExtension: "")
                do {
                    try fileManager.moveItem(at: directory, to: targetUrl)
                    _ = Game(name: name, directory: targetUrl.lastPathComponent, insertInto: context, importFiles: true)
                }
                catch { }
            }
            try context.save()
        }
        catch { }
    }
    
    func deleteExpiredGames(context: NSManagedObjectContext) {
        do {
            var directories = [String]()
            
            let fetchRequest = NSFetchRequest<NSFetchRequestResult>(entityName: "Game")
            let cutoffDate = Date(timeIntervalSinceNow: -30 * 24 * 60 * 60)
            fetchRequest.predicate = NSPredicate(format: "deletedDate < %@", argumentArray: [ cutoffDate ])

            guard let results = try context.fetch(fetchRequest) as? [Game] else { return }

            for game in results {
                print("deleting game \(game.name)")
                directories.append(game.directory)
                context.delete(game)
            }
            
            try context.save()
            
            let fileManager = FileManager.default
            let libraryUrl = AppDelegate.libraryURL
            
            for directory in directories {
                do {
                    print("deleting directory \(directory)")
                    try fileManager.removeItem(at: libraryUrl.appendingPathComponent(directory))
                }
                catch { }
            }
        }
        catch { }
    }
}
